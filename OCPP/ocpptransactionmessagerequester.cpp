#include "ocpptransactionmessagerequester.h"
#include "logconfig.h"
#include "gvar.h"
#include "ccharger.h"

OcppTransactionMessageRequester::OcppTransactionMessageRequester(EvChargingTransactionRegistry &registry,
                                                 OcppClient &ocppCli,
                                                 QObject *parent) :
    QObject(parent),
    reg(registry),
    cli(ocppCli),
    sendProgress(SendOcppProgress::Idle),
    meterValIdx(-1)
{
    connect(&cli, &OcppClient::startTransactionResponse, this, &OcppTransactionMessageRequester::onStartTransactionResponded);
    connect(&cli, &OcppClient::stopTransactionResponse, this, &OcppTransactionMessageRequester::onStopTransactionResponded);
    connect(&cli, &OcppClient::meterValueResponse, this, &OcppTransactionMessageRequester::onMeterValueResponded);

    connect(&reg, &EvChargingTransactionRegistry::chargingStarted, this, &OcppTransactionMessageRequester::onChargingStarted);
    connect(&reg, &EvChargingTransactionRegistry::chargingResumed, this, &OcppTransactionMessageRequester::onChargingResumed);
    connect(&reg, &EvChargingTransactionRegistry::meterValUpdated, this, &OcppTransactionMessageRequester::onMeterValUpdated);
    connect(&reg, &EvChargingTransactionRegistry::chargingCanceled, this, &OcppTransactionMessageRequester::onChargingCanceled);
    connect(&reg, &EvChargingTransactionRegistry::chargingFinished, this, &OcppTransactionMessageRequester::onChargingFinished);
}

void OcppTransactionMessageRequester::reset() // after transactionError
{
    curTrans.clear();
    sendProgress = SendOcppProgress::Idle;
    meterValIdx = -1;
    // unable to stop the one-shot timer from scheduleProcess()
}

bool OcppTransactionMessageRequester::nextTransaction()
{
    bool hasTransToProcess = false;
    auto trans = reg.getOldest();
    try {
        if (!trans) {
            throw std::runtime_error("There is no transaction to process");
        }
        if (cli.isOffline()) {
            throw std::runtime_error("WebSocket is currently offline - cannot process transactions.");
        }
        if (m_ChargerA.ConnType() == CCharger::ConnectorType::WallSocket) {
            throw std::runtime_error("m_ChargerA.ConnType() != CCharger::ConnectorType::WallSocket");
        }
        if (sendProgress == SendOcppProgress::Idle) {
            curTrans = trans;
            meterValIdx = -1;
            scheduleProcess();
            hasTransToProcess = true;
        }
        else {
            qCWarning(Transactions) << "OcppTransactionMessageRequester: nextTransaction() called at state "
                                    << static_cast<int>(sendProgress);
        }
        switch(sendProgress) {
        case SendOcppProgress::StartTx:
        case SendOcppProgress::StartTxWaitReply:
        case SendOcppProgress::Meter:
        case SendOcppProgress::MeterWaitReply:
        case SendOcppProgress::StopTx:
        case SendOcppProgress::StopTxWaitReply:
            hasTransToProcess = true;
            break;
        case SendOcppProgress::Idle:
            // do no override the previous value here
            break;
        default:
            // do no override the previous value here
            hasTransToProcess = false;
            break;
        }
    }
    catch (std::exception &e) {
        qCWarning(Transactions) << "OcppTransactionMessageRequester: " << e.what();
    }

    return hasTransToProcess;
}

bool OcppTransactionMessageRequester::isProcessingInactiveTrans()
{
    bool status = false;

    auto trans = reg.getOldest();
    if (trans) {
        if (!trans->isActive()) {
            switch(sendProgress) {
            case SendOcppProgress::Idle: // expect some action very soon
            case SendOcppProgress::StartTx:
            case SendOcppProgress::StartTxWaitReply:
            case SendOcppProgress::Meter:
            case SendOcppProgress::MeterWaitReply:
            case SendOcppProgress::StopTx:
            case SendOcppProgress::StopTxWaitReply:
                status = true;
                break;
            default:
                // do no override the previous value here
                status = false;
                break;
            }
        }
    }

    return status;
}

void OcppTransactionMessageRequester::process()
{
    try {
        qCDebug(Transactions) << "OcppTransactionMessageRequester::process() sendProgress=" << static_cast<int>(sendProgress);
        switch (sendProgress) {
        case SendOcppProgress::Idle:
            if (curTrans) {
                if (!curTrans->hasTransactionId()) {
                    // send Start Transaction
                    sendProgress = SendOcppProgress::StartTx;
                    scheduleProcess();
                }
                else {
                    // already has transaction Id but not yet in Meter or other states.
                    // because transaction was loaded from file.
                    qCDebug(Transactions()) << "Transaction already has Transaction Id. Is this resumed from system-restart?";
                    if (curTrans->hasSentMeterValIdx()) {
                        qCDebug(Transactions()) << "Transaction already has LastSentMeterValIdx " <<  curTrans->getLastSentMeterValIdx();
                        meterValIdx = curTrans->getLastSentMeterValIdx() - 1;
                    }
                    sendProgress = SendOcppProgress::Meter;
                    scheduleProcess();
                }
            }
            break;
        case SendOcppProgress::StartTx:
            // make StartTransaction.req
            if (curTrans->hasAuthorizationId()) {
                cli.addStartTransactionReq(curTrans->getTransactionStartDateTime(),
                                           curTrans->getTransactionStartMeterQValue(),
                                           curTrans->getAuthorizationId());
                sendProgress = SendOcppProgress::StartTxWaitReply;
                scheduleProcess();
            }
            else {
                throw std::runtime_error("OcppTransactionMessageRequester cannot make StartTransaction.req because there is no idTag value.");
            }
            break;
        case SendOcppProgress::StartTxWaitReply:
            // wait for StartTransaction.conf
            break;
        case SendOcppProgress::Meter:
        {
            // make MeterValue.req
            if (!curTrans->hasTransactionId()) {
                throw std::runtime_error("OcppTransactionMessageRequester cannot make MeterValues.req because there is no transaction-Id value.");
            }
            int nextMeterValIdx = (meterValIdx < 0)? 0 : (meterValIdx + 1);
            bool found = false;
            EvChargingDetailMeterVal meterVal = curTrans->getMeterVal(nextMeterValIdx, &found);
            if (found) {
                cli.addMeterValuesReq(curTrans->getTransactionId(), meterVal);
                sendProgress = SendOcppProgress::MeterWaitReply;
                scheduleProcess();
            }
            else {
                if (curTrans->isActive()) {
                    //Keep in this state, wait for the next one
                }
                else {
                    // That's the end of meter-values, move-on
                    sendProgress = SendOcppProgress::StopTx;
                    scheduleProcess();
                }
            }
        }
            break;
        case SendOcppProgress::MeterWaitReply:
            // wait for MeterValue.conf
            break;
        case SendOcppProgress::StopTx:
            // make StopTransaction.req if already transaction is canceled/finished.
            // otherwise wait until it becomes so.
            if (!curTrans->isActive()) {
                if (curTrans->hasTransactionId() && curTrans->hasAuthorizationId()) {
                    cli.addStopTransactionReq(curTrans->getTransactionId(), curTrans->getTransactionEndDateTime(),
                                              curTrans->getTransactionEndMeterQValue(), curTrans->getTransactionEndReason(),
                                              curTrans->getAuthorizationId());
                    sendProgress = SendOcppProgress::StopTxWaitReply;
                    scheduleProcess();
                }
                else {
                    qCDebug(Transactions) << "At StopTx - condition failed." << " hasTransId " << curTrans->hasTransactionId()
                                          << " hasAuthId " << curTrans->hasAuthorizationId();
                    throw std::runtime_error("Null-Value TransactionId cannot make StopTransaction.req");
                }
            }
            else {
                // Keep in this state, wait active transaction to end
                qCDebug(Transactions) << "At StopTx - no change remain in this state";
            }
            break;
        case SendOcppProgress::StopTxWaitReply:
            // wait for StopTransaction.conf
            break;
        case SendOcppProgress::Completed:
            // done - emit transactionCompleted
            emit transactionCompleted(curTrans->getTransactionId());
            sendProgress = SendOcppProgress::Idle;
            scheduleProcess();
            break;
        case SendOcppProgress::Error:
            // emit transactionError?
            emit transactionError(curTrans->getAuthorizationId(), curTrans->getTransactionId());
            break;
        }
    }
    catch (std::exception &e) {
        qCWarning(Transactions) << "OcppTransactionMessageRequester::process() error "
                                << e.what();
        sendProgress = SendOcppProgress::Error;
        scheduleProcess();
    }

}

void OcppTransactionMessageRequester::scheduleProcess()
{
    qCDebug(Transactions) << "OcppTransactionMessageRequester::scheduleProcess() sendProgress=" << static_cast<int>(sendProgress);
    QTimer::singleShot(0, this, &OcppTransactionMessageRequester::process);
}

void OcppTransactionMessageRequester::onStartTransactionResponded(ocpp::AuthorizationStatus status, const QString idTag, const QString transId)
{
    qCDebug(Transactions) << "OcppTransactionMessageRequester::onStartTransactionResponded() " << transId;
    try {
        switch (sendProgress) {
        case SendOcppProgress::StartTxWaitReply:
            switch (status) {
            case ocpp::AuthorizationStatus::Accepted:
            case ocpp::AuthorizationStatus::ConcurrentTx:
            case ocpp::AuthorizationStatus::Expired:
            case ocpp::AuthorizationStatus::Blocked:
            case ocpp::AuthorizationStatus::Invalid:
                if (curTrans->hasAuthorizationId()) {
                    if (idTag.compare(curTrans->getAuthorizationId()) == 0) {
                        curTrans->setTransactionId(transId);
                    }
                    else {
                        throw std::runtime_error("Transaction Authorization-ID mismatch: " " expected: "
                                                 + curTrans->getAuthorizationId().toStdString()
                                                 + " received: " + idTag.toStdString());
                    }
                }
                else {
                    throw std::runtime_error("Transaction missing expected Authorization-ID");
                }
                sendProgress = SendOcppProgress::Meter;
                meterValIdx = -1;
                scheduleProcess();
                break;
            default:
                // let others handle this.
                throw std::runtime_error("Authorization rejected.");
                break;
            }
            break;
        default:
            qCWarning(Transactions) << "OcppTransactionMessageRequester::onStartTransactionResponded: unexpected state "
                                    << static_cast<int>(sendProgress);
            break;
        }
    }
    catch (std::exception &ex) {
        qCWarning(Transactions) << "OcppTransactionMessageRequester::onStartTransactionResponded: " << ex.what();
        sendProgress = SendOcppProgress::Error;
        scheduleProcess();
    }
}

void OcppTransactionMessageRequester::onStopTransactionResponded(ocpp::AuthorizationStatus status, const QString transId)
{
    qCDebug(Transactions) << "OcppTransactionMessageRequester::onStopTransactionResponded() " << transId;
    try {
        switch (sendProgress) {
        case SendOcppProgress::StopTxWaitReply:
            switch (status) {
            case ocpp::AuthorizationStatus::Accepted:
            case ocpp::AuthorizationStatus::ConcurrentTx:
                Q_ASSERT(curTrans->hasTransactionId());
                if (curTrans->getTransactionId().compare(transId) != 0) {
                    throw std::runtime_error("Transaction ID mismatch: " " expected: "
                                             + curTrans->getTransactionId().toStdString()
                                             + " received: " + transId.toStdString());
                }
                sendProgress = SendOcppProgress::Completed;
                scheduleProcess();
                break;
            default:
                // let others handle this.
                throw std::runtime_error("Authorization rejected.");
                break;
            }
            break;
        default:
            qCWarning(Transactions) << "OcppTransactionMessageRequester::onStopTransactionResponded: unexpected state "
                                    << static_cast<int>(sendProgress);
            break;
        }
    }
    catch (std::exception &ex) {
        qCWarning(Transactions) << "OcppTransactionMessageRequester::onStopTransactionResponded: " << ex.what();
        sendProgress = SendOcppProgress::Error;
        scheduleProcess();
    }
}

void OcppTransactionMessageRequester::onMeterValueResponded(const QString transId)
{
    qCDebug(Transactions) << "OcppTransactionMessageRequester::onMeterValueResponded() " << transId;
    try {
        switch (sendProgress) {
        case SendOcppProgress::MeterWaitReply:
            Q_ASSERT(curTrans->hasTransactionId());
            if (curTrans->getTransactionId().compare(transId) != 0) {
                throw std::runtime_error("Transaction ID mismatch: " " expected: "
                                         + curTrans->getTransactionId().toStdString()
                                         + " received: " + transId.toStdString());
            }
            if (meterValIdx < 0)
                meterValIdx = 0;
            else
                meterValIdx += 1;
            // this is sole for the sake of recovering from system-restart before these messages are sent to CS.
            curTrans->updateLastSentMeterValIdx(meterValIdx);
            sendProgress = SendOcppProgress::Meter;
            scheduleProcess();
            break;
        default:
            qCWarning(Transactions) << "OcppTransactionMessageRequester::onMeterValueResponded: unexpected state "
                                    << static_cast<int>(sendProgress);
            break;
        }
    }
    catch (std::exception &ex) {
        qCWarning(Transactions) << "OcppTransactionMessageRequester::onMeterValueResponded: " << ex.what();
        sendProgress = SendOcppProgress::Error;
        scheduleProcess();
    }
}


void OcppTransactionMessageRequester::onChargingStarted(const QSharedPointer<EvChargingTransaction>) // New Transaction added without Transaction-ID
{
    // Nothing to do here
    qCDebug(Transactions) << "OcppTransactionMessageRequester::onChargingStarted: nothing to do";
}

void OcppTransactionMessageRequester::onChargingResumed(const QSharedPointer<EvChargingTransaction> trans) // Previous Transaction resumed with existing Transaction-ID
{
    qCDebug(Transactions) << "OcppTransactionMessageRequester::onChargingResumed: SendProgress = " << static_cast<int>(sendProgress);
    switch (sendProgress) {
    case SendOcppProgress::Idle:
        if (trans->isActive() && trans->hasTransactionId()) {
            curTrans = trans;
            sendProgress = SendOcppProgress::Meter;
            scheduleProcess();
        }
        break;
    default:
        break;
    }
}

bool OcppTransactionMessageRequester::isWaitingForMeterValue(const QSharedPointer<EvChargingTransaction> trans)
{
    bool waiting = false;
    if ((sendProgress == SendOcppProgress::Meter) && (trans == curTrans)) {
        if (meterValIdx < 0) {
            waiting = true;
        }
        else if ((curTrans->getNumMeterVals() - meterValIdx) <= 2) {
            waiting = true;
        }
    }
    return waiting;
}

void OcppTransactionMessageRequester::onMeterValUpdated(const QSharedPointer<EvChargingTransaction> trans) // Meter Value updated
{
    qWarning()<<"Current EvChargingTransaction:"<<EvChargingTransaction::currentTransc<<((EvChargingTransaction::currentTransc==nullptr)?-1:EvChargingTransaction::currentTransc->getTransactionId2());
    // only schedule to process it if the object is waiting for the next meter-value.
    if (isWaitingForMeterValue(trans)) {
        qCDebug(Transactions) << "OcppTransactionMessageRequester waiting for MeterValues. Schedule to process now.";
        scheduleProcess();
    }
}

void OcppTransactionMessageRequester::onChargingCanceled(const QSharedPointer<EvChargingTransaction> trans) // Transaction canceled before becoming Active
{
    // only schedule to process it if the object is waiting for the next meter-value.
    if (isWaitingForMeterValue(trans)) {
        qCDebug(Transactions) << "OcppTransactionMessageRequester::onChargingCanceled() waiting for MeterValues. Schedule to process now.";
        sendProgress = SendOcppProgress::StopTx;
        scheduleProcess();
    }
}

void OcppTransactionMessageRequester::onChargingFinished(const QSharedPointer<EvChargingTransaction> trans) // Transaction finished
{
    // only schedule to process it if the object is waiting for the next meter-value.
    if (isWaitingForMeterValue(trans)) {
        qCDebug(Transactions) << "OcppTransactionMessageRequester::onChargingFinished() waiting for MeterValues. Schedule to process now.";
        sendProgress = SendOcppProgress::StopTx;
        scheduleProcess();
    }
}
