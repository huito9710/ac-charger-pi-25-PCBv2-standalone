#include "chargerrequestrfid.h"
#include "hostplatform.h"
#include "gvar.h"
#include "setchargingtimewin.h"
#include "localauthlist.h"
#include "statusledctrl.h"
#include "api_function.h"

ChargerRequestRfid::ChargerRequestRfid(QObject *parent) :
    QObject(parent),
    st(State::Idle),
    readerWin(nullptr),
    purpose(Purpose::BeginCharge)
{
    // must only connect only once to avoid multiple callbacks
    connect(&m_OcppClient, &OcppClient::OcppAuthorizeResponse, this, &ChargerRequestRfid::onOcppAuthorizeResponse);
    connect(&m_OcppClient, &OcppClient::OcppAuthorizeReqCannotSend, this, &ChargerRequestRfid::onOcppAuthorizeReqCannotSend);
}

ChargerRequestRfid::~ChargerRequestRfid()
{
    closeReaderWin();
}

void ChargerRequestRfid::setStartTxRfid(const QString uuid)
{
    startTxRfidUuid = uuid;
}

void ChargerRequestRfid::start(Purpose request, bool showWin)
{
    switch (st) {
    case State::Idle:
        Q_ASSERT(!readerWin);
        readerWin = new RfidReaderWindow(reinterpret_cast<QWidget*>(parent()));
        connect(readerWin, &RfidReaderWindow::readerRespond, this, &ChargerRequestRfid::onReaderResponded);
        readerWin->setWindowState(charger::Platform::instance().defaultWinState);
        if (showWin)
            readerWin->show();
        readerWin->askForCard(request == Purpose::BeginCharge?
                RfidReaderWindow::Purpose::StartCharge : (request == Purpose::EndCharge
                               ?RfidReaderWindow::Purpose::StopCharge
                               :RfidReaderWindow::Purpose::SuspendCharge));
        st = State::ReadCard;
        pCurWin = readerWin;
        purpose = request;
        break;
    case State::ReadCard:
    case State::AuthSucceeded:
    case State::AuthRejected:
        Q_ASSERT(readerWin);
        if (showWin && !readerWin->isVisible())
            readerWin->show();
        else if (!showWin && readerWin->isVisible())
            readerWin->hide();
        break;
    case State::OcppAuthorize:
        Q_ASSERT(readerWin);
        if (showWin)
            readerWin->show();
        else if (!showWin)
            readerWin->hide();
        break;
    default:
        qWarning() << "unexpected start() request at this ChargerRequestRfid state.";
        Q_ASSERT(false);
    }
}

void ChargerRequestRfid::runBackground()
{
    readerWin->hide();
}

void ChargerRequestRfid::stop()
{
    if (readerWin) {
        readerWin->requestClose();
        readerWin = nullptr;
    }
    st = State::Idle;
}

 void ChargerRequestRfid::checkCardAgainstServer(const QString uuid, const bool useAuthCache)
 {
     st = State::OcppAuthorize;

     // OCPP-Authorize
     m_ChargerA.IdTag = uuid;
     m_OcppClient.addAuthorizeReq(m_ChargerA.IdTag, useAuthCache);

     // Update Display Message
     readerWin->displayMessage(RfidReaderWindow::Message::AuthStarted, false);
 }

void ChargerRequestRfid::onReaderResponded(RfidReaderWindow::ReaderStatus newStatus, const QByteArray &cardNum)
{
    using RS = RfidReaderWindow::ReaderStatus;

    switch (newStatus) {
    case RS::CardReadOK:
    {
        QString cardNumStr = cardNum.toHex().toUpper();

        if (purpose == Purpose::EndCharge) {
            if (startTxRfidUuid.size() > 0 && cardNumStr.compare(startTxRfidUuid) == 0) {
                // Accept
                m_ChargerA.IdTag = cardNumStr;
                emit requestAccepted();
            }
            else if (startTxRfidUuid.size() > 0){
                qWarning() << "NFC Card UUID to stop charging does not match the one used for start charging";
                // TODO: Determine how to handle this case. Reject case for Airport Deployment July-2020.
                if(charger::GetAuthStop())
                {
                    checkCardAgainstServer(cardNumStr, false);
                } else {
                    st = State::AuthRejected;
                    StatusLedCtrl::instance().setState(StatusLedCtrl::State::RFIDError);
                    readerWin->displayMessage(RfidReaderWindow::Message::StopTxnCardNotMatch, true);
                }
            }
            else {
                qWarning() << "NFC Card UUID used for start charging is empty value. Allow all cards to stop";
                // Accept
                m_ChargerA.IdTag = cardNumStr;
                emit requestAccepted();
            }
        }
        else if(purpose == Purpose::BeginCharge){ // BeginCharge
            std::unique_ptr<ocpp::AuthorizationStatus> localAuthStatus = checkLocalAuthList(cardNumStr);
            if (localAuthStatus) {
                switch (*localAuthStatus) {
                case ocpp::AuthorizationStatus::Accepted:
                case ocpp::AuthorizationStatus::ConcurrentTx:
                    m_ChargerA.IdTag = cardNumStr;
                    emit requestAccepted();
                    break;
                default:
                    RfidReaderWindow::RejectReason reason = RfidReaderWindow::RejectReason::Nil;
                    switch (*localAuthStatus) {
                    case ocpp::AuthorizationStatus::Expired: reason = RfidReaderWindow::RejectReason::Expired; break;
                    case ocpp::AuthorizationStatus::Blocked: reason = RfidReaderWindow::RejectReason::TempBlocked; break;
                    case ocpp::AuthorizationStatus::Invalid: reason = RfidReaderWindow::RejectReason::BlackListed; break;
                    default:
                        assert(0);
                    }
                    st = State::AuthRejected;
                    StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashingred);
                    displayRejection(reason);
                    break;
                }
            }
            else { // Card Number not present in Local Authorization List
                checkCardAgainstServer(cardNumStr);
            }
        }else if(purpose == Purpose::SuspendCharge){
            if(IsValidCard(cardNumStr)||(m_ChargerA.IdTag == cardNumStr)){
                emit requestAccepted();
            }else{
                emit requestCanceled();
            }
        }
    }
        break;
    case RS::CardError:
        // Display Error
        break;
    case RS::UserCancel:
        // Do not close RfidReaderWindow - unless instructed by caller
        emit requestCanceled();
        break;
    }
}

std::unique_ptr<ocpp::AuthorizationStatus> ChargerRequestRfid::checkLocalAuthList(QString tagUid)
{
    LocalAuthList &authList = LocalAuthList::getInstance();
    return authList.getAuthStatusById(tagUid.toStdString());
}

void ChargerRequestRfid::onOcppAuthorizeResponse(bool responseAvail, ocpp::AuthorizationStatus authStatus)
{
    bool accepted = false;

    switch (purpose) {
    case Purpose::BeginCharge:
        if (responseAvail && authStatus == ocpp::AuthorizationStatus::Accepted)
            accepted = true;
        else
            accepted = false;
        break;
    case Purpose::EndCharge:
        if (responseAvail) {
            switch (authStatus) {
            case ocpp::AuthorizationStatus::Accepted:
            case ocpp::AuthorizationStatus::ConcurrentTx:
                accepted = true;
                break;
            default:
                accepted = false;
                break;
            }
        }
    }

    if (accepted) {
        st = State::AuthSucceeded;
        emit requestAccepted();
    }
    else {
        using Reason = RfidReaderWindow::RejectReason;
        Reason r = Reason::Nil;
        if (responseAvail) {
            switch(authStatus) {
            case ocpp::AuthorizationStatus::Expired: r = RfidReaderWindow::RejectReason::Expired; break;
            case ocpp::AuthorizationStatus::Blocked: r = RfidReaderWindow::RejectReason::TempBlocked; break;
            case ocpp::AuthorizationStatus::Invalid: r = RfidReaderWindow::RejectReason::BlackListed; break;
            default:
                r = Reason::Nil;
                break;
            }
        }
        st = State::AuthRejected;
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashingred);
        displayRejection(r);
    }
}

void ChargerRequestRfid::onOcppAuthorizeReqCannotSend()
{
    st = State::AuthRejected;
    StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashingred);
    readerWin->displayMessage(RfidReaderWindow::Message::ServerOffline, RfidReaderWindow::RejectReason::Nil, true);
}

void ChargerRequestRfid::displayRejection(RfidReaderWindow::RejectReason reason){
    switch (purpose) {
    case Purpose::BeginCharge:
        readerWin->displayMessage(RfidReaderWindow::Message::StartRejectedTryAgain, reason, true);
        break;
    case Purpose::EndCharge:
        readerWin->displayMessage(RfidReaderWindow::Message::StopRejectedTryAgain, reason, true);
        break;
    }
}
