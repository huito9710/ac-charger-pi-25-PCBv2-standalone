#ifndef OCPPTRANSACTIONMESSAGEREQUESTER_H
#define OCPPTRANSACTIONMESSAGEREQUESTER_H

#include <QObject>
#include "evchargingtransactionregistry.h"
#include "OCPP/ocppoutmessagequeue.h"
#include "OCPP/ocppclient.h"

///
/// \brief The OcppTransactionMessageRequester class
///
/// \abstract Make ocpp requests from given EvChargingTransactionRegistry.
///           Starting with the oldest transaction. Send the corresponding
///           StartTransaction.req, StopTransaction.req and MeterValue.req
///           with the given OcppOutMessageQueue. One-transaction-at-a-time.
///
///
class OcppTransactionMessageRequester : public QObject
{
    Q_OBJECT
public:
    explicit OcppTransactionMessageRequester(EvChargingTransactionRegistry &registry,
                                             OcppClient &ocppCli,
                                             QObject *parent = nullptr);
    void reset(); // after transactionError
    bool nextTransaction(); // next (oldest) transaction from registry is ready to process.
    bool isProcessingInactiveTrans();

signals:
    void transactionCompleted(const QString transactionId); // Start, MeterValue(s), Stop
    void transactionError(const QString idTag, const QString transactionId);

private slots:
    void onStartTransactionResponded(ocpp::AuthorizationStatus status, const QString idTag, const QString transId = QString());
    void onStopTransactionResponded(ocpp::AuthorizationStatus status, const QString transId);
    void onMeterValueResponded(const QString transId);

    // only makes sense if the current transaction is 'active'
    void onChargingStarted(const QSharedPointer<EvChargingTransaction>); // New Transaction added without Transaction-ID
    void onChargingResumed(const QSharedPointer<EvChargingTransaction>); // Previous Transaction resumed with existing Transaction-ID
    void onMeterValUpdated(const QSharedPointer<EvChargingTransaction>); // Meter Value updated
    void onChargingCanceled(const QSharedPointer<EvChargingTransaction>); // Transaction canceled before becoming Active
    void onChargingFinished(const QSharedPointer<EvChargingTransaction>); // Transaction finished

private:
    // Which of the messages has been sent
    enum class SendOcppProgress {
        Idle,
        StartTx,
        StartTxWaitReply,
        Meter,
        MeterWaitReply,
        StopTx,
        StopTxWaitReply,
        Completed,
        Error
    };

private:
    EvChargingTransactionRegistry &reg;
    OcppClient &cli;
    SendOcppProgress sendProgress;
    int meterValIdx; // last confirmed MeterValues.req
    QSharedPointer<EvChargingTransaction> curTrans;  //cursor

private slots:
    void process();

private:
    void scheduleProcess();
    bool isWaitingForMeterValue(const QSharedPointer<EvChargingTransaction> trans);

};

#endif // OCPPTRANSACTIONMESSAGEREQUESTER_H
