#include "ocppclient.h"
#include <QFile>
#include <QMessageBox>
#include <QHostAddress>
#include <QJsonDocument>
#include <iostream>
#include "api_function.h"
#include "ocppconst.h"
#include "ccharger.h"
#include "const.h"
#include "ocppTypes.h"
#include "rpcmessage.h"
#include "callmessage.h"
#include "callresultmessage.h"
#include "callerrormessage.h"
#include "logconfig.h"
#include "localauthlist.h"
#include "chargerconfig.h"
#include "firmwareupdatescheduler.h"
#include "gvar.h"
#include "distancesensorthread.h"

QT_USE_NAMESPACE

//! [constructor]
OcppClient::OcppClient(QObject *parent) :
    QObject(parent),
    chargerRegStatus(ocpp::RegistrationStatus::Unknown),
    ws(),
    ocppMsgOutQ(ws),
    authCache(AuthorizationCache::getInstance()),
    defaultHeartBeatInterval(60*1000), // 1-minute
    hbTimerInterval(defaultHeartBeatInterval),
    connUnavailPending(false),
    connMaxWait(30),
    defaultBootNotifRetryInterval(60),
    authCacheFlushInterval(1),
    meterIntervalKeyValue(defaultMeterValueSampleIntervalValue),    //0 is no transmit
    defaultHeartbeatTriggerCount(15)
{
    m_CableTimeoutKeyValue =600*2;

    heartBeatTimer.setSingleShot(false);
    heartBeatTimer.setInterval(hbTimerInterval);
    heartbeatTriggerCountTimer.setSingleShot(false);
    heartbeatTriggerCountTimer.setInterval(1000);
    connect(&heartBeatTimer, &QTimer::timeout, this, &OcppClient::onHeartBeatTimeout);
    connect(&heartbeatTriggerCountTimer, &QTimer::timeout, this, &OcppClient::incrHeartbeatTriggerCount);
    connect(&ws, &QWebSocket::connected, this, &OcppClient::onConnected);
    connect(&ws, &QWebSocket::disconnected, this, &OcppClient::onDisconnected);
    // QWebSocket::error is a overloaded function, refer to Qt manual of how this works
    QObject::connect(&ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            [=](QAbstractSocket::SocketError error) {
                qCWarning(OcppComm) << "WebSocket Socket-Error: " << error << ". " << ws.errorString();
                switch (st){
                case State::Connecting:
                    emit connectFailed();
                    break;
                default:
                    break;
                }
            });
    connect(&ws, &QWebSocket::stateChanged, this, &OcppClient::onStateChanged);
    connect(&ws, &QWebSocket::preSharedKeyAuthenticationRequired, this, &OcppClient::onPreSharedKeyAuthRequired);
    connect(&ws, &QWebSocket::proxyAuthenticationRequired, this, &OcppClient::onProxyAuthenticationRequired);
    typedef void (QWebSocket:: *sslErrorsSignal)(const QList<QSslError> &);
    connect(&ws, static_cast<sslErrorsSignal>(&QWebSocket::sslErrors), this, &OcppClient::onSslErrors);
    connect(&ws, &QWebSocket::textMessageReceived, this, &OcppClient::onTextMessageReceived);

    connTimer.setSingleShot(true);
    connect(&connTimer, &QTimer::timeout, this, &OcppClient::onWsTimeout);
    bootNotifTimer.setSingleShot(true);
    connect(&bootNotifTimer, &QTimer::timeout, this, &OcppClient::onBootNotifTimeout);

    connect(&ocppMsgOutQ, &OcppOutMessageQueue::msgSent, this, &OcppClient::onMsgSent);
    connect(&ocppMsgOutQ, &OcppOutMessageQueue::msgError, this, &OcppClient::onMsgError);

    // Authorization Cache is flushed whenever this is an update, based on feedbacks from CS or self-managed (out-of-space).
    // Since the cache is to improve responsiveness to CS, we will defer to flush-to-disk until some time after the trigger event.
    // The assumption is writing to disk is slow, especially on NAND Flash.
    authCacheFlushTimer.setSingleShot(true);
    authCacheFlushTimer.setInterval(this->authCacheFlushInterval);
    connect(&authCacheFlushTimer, &QTimer::timeout, this, &OcppClient::onAuthCacheFlushTimeout);
    authCache.setMaxCacheEntries(charger::getAuthorizationCacheMaxSize());

    connect(&m_ChargerA, &CCharger::connAvailabilityChanged, this, &OcppClient::onConnAvailChanged);

    connect(&connectionFailTimer, &QTimer::timeout, this, &OcppClient::onConnectionFailTimeout);
}
//! [constructor]

OcppClient::~OcppClient()
{
    authCache.flushToDisk();
}

void OcppClient::onMsgSent(QSharedPointer<ocpp::RpcMessage> sentMsg)
{
    switch (sentMsg->getType()) {
    case ocpp::RpcMessageType::Call:
        st = State::CallMessageSent;
        break;
    case ocpp::RpcMessageType::CallResult:
    case ocpp::RpcMessageType::CallError:
        // Do nothing
        break;
    }
}

void OcppClient::onMsgError(OcppOutMessageQueue::MessageSendError error, QSharedPointer<ocpp::RpcMessage> outMsg)
{
    qCWarning(OcppComm) << "OcppClient: Send Message Error. "
                        << "Id: " << outMsg->getId().data()
                        << "; MessageSendError Code " << static_cast<int>(error);

    if (error == OcppOutMessageQueue::MessageSendError::ServerOffline
        && outMsg->getType() == ocpp::RpcMessageType::Call) {
            QSharedPointer<ocpp::CallMessage> callMsg = outMsg.dynamicCast<ocpp::CallMessage>();
            if (callMsg->getAction() == ocpp::callActionStrs.at(ocpp::CallActionType::Authorize)) {
                emit OcppAuthorizeReqCannotSend();
            }
    }

    emit callTimedout();
}

void OcppClient::onMsgNoReplyError(QSharedPointer<ocpp::RpcMessage> msg)
{
    qCWarning(OcppComm) << "ocpp::CallActionType::StopTransactiowxxxx onMsgNoReplyError";
    QSharedPointer<ocpp::RpcMessage> matchingRpcMsg  = ocppMsgOutQ.removeSentCallMessage(msg->getId().data());
    std::istringstream iss(matchingRpcMsg.dynamicCast<ocpp::CallMessage>()->getAction());
    ocpp::CallActionType callAction;
    iss >> callAction;

    switch(callAction) {
        case ocpp::CallActionType::StopTransaction:
            try {
                std::string stopTransactionMsg = "[3,\"0\",{\"idTagInfo\":{\"status\":\"Accepted\"}}]";
                std::shared_ptr<ocpp::RpcMessage> rpcMsg = ocpp::CallResultMessage::fromMessageString(stopTransactionMsg);
                handleCallResultMessage(
                            std::dynamic_pointer_cast<ocpp::CallResultMessage>(rpcMsg),
                            matchingRpcMsg.dynamicCast<ocpp::CallMessage>());
            }
            catch (std::runtime_error &rerr) {
                qCWarning(OcppComm) << "StopTransaction.conf: " << rerr.what();
            }
            break;
        default:
           onConnectionFailTimeout();
           qCWarning(OcppComm) << "Unknown call action:" << callAction;
           break;
    }
}

void OcppClient::onWsTimeout()
{
    switch (st) {
    case State::Connecting:
        ws.abort(); // Abort the connection
        st = State::Idle;
        qCWarning(OcppComm) << QString("Connecting to Server takes longer than %1 secs - Abort.").arg(connMaxWait.count()).toUtf8();
        emit connectFailed();
        break;
    default:
        qCWarning(OcppComm) << QString("onWsTimeout() at unexpected state(%1)").arg(static_cast<int>(st)).toUtf8();
        emit connectFailed();
        break;
    }
}

void OcppClient::onBootNotifTimeout()
{
    switch (chargerRegStatus){
    case ocpp::RegistrationStatus::Pending:
    case ocpp::RegistrationStatus::Rejected:
        ocppSendBootNotification();
        break;
    default:
        qCWarning(OcppComm) << "Unexpected boot-notification timer handler when RegistrationStatus is ("
                            << static_cast<int>(chargerRegStatus) << ").";
        break;
    }
}

void OcppClient::onAuthCacheFlushTimeout()
{
    authCache.flushToDisk();
}

void OcppClient::onAuthCacheLookupSucc()
{
    emit OcppAuthorizeResponse(true, ocpp::AuthorizationStatus::Accepted);
}

void OcppClient::onConnAvailChanged(bool newVal)
{
    Q_UNUSED(newVal);
    connUnavailPending = false; // ocpp::ChangeAvailabililty request is executed.
}

//! [onConnected]
void OcppClient::onConnected()
{
    qWarning() << "WS connected";
    using namespace ocpp;

    switch (st) {
    case State::Connecting:
        connTimer.stop();
        st = State::Connected;
        emit connected();
        switch(chargerRegStatus) {
        case RegistrationStatus::Unknown:
        case RegistrationStatus::Pending:
        case RegistrationStatus::Rejected:
            ocppSendBootNotification();
            break;
        default:
            // Already accepted. Do nothing
            break;
        }
        break;
    default:
        qCWarning(OcppComm) << "Unexpectedly received 'Connected' signal in State (" << static_cast<int>(st) << ")";
        break;
    }
}
//! [onConnected]

void OcppClient::onDisconnected()
{
    // switch (st) {
    // case State::Disconnecting:
    //     st = State::Idle;
    //     qCWarning(OcppComm) << "WebSocket Disconnected.";
    //     break;
    // default:
    //     qCWarning(OcppComm) << "WebSocket Disconnected unexpectedly (State="
    //                         << static_cast<int>(st) << ").";
    //     st = State::Idle;
    //     break;
    // }
    // // Stop all Timers
    // connTimer.stop();
    // bootNotifTimer.stop();

    // emit disconnected();
    qWarning() << "WS disconnected";
    stopHeartbeatTimer();
    st = State::Idle;
    emit callTimedout();
}

void OcppClient::stopHeartbeatTimer()
{
    if (heartBeatTimer.isActive())
        heartBeatTimer.stop();
    if (heartbeatTriggerCountTimer.isActive())
        heartbeatTriggerCountTimer.stop();
    heartbeatTriggerCount = 0;
}

void OcppClient::onStateChanged(QAbstractSocket::SocketState sockState)
{
    qCDebug(OcppComm) << "WebSocket State-Changed: " << sockState;
}

void OcppClient::onPreSharedKeyAuthRequired(QSslPreSharedKeyAuthenticator *authenticator)
{
    Q_UNUSED(authenticator);

    qCWarning(OcppComm) << "WebSocket Connection Pre-Shared-Key Authentication Requested - Not Handled.";
}

void OcppClient::onProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    Q_UNUSED(proxy);
    Q_UNUSED(authenticator);

    qCWarning(OcppComm) << "WebSocket Proxy-Authentication Requested - Not Handled.";
}

void OcppClient::onHeartBeatTimeout()
{
    ocppSendHeartbeat();
}

//! [onTextMessageReceived]
void OcppClient::onTextMessageReceived(QString message)
{
    qCDebug(OcppComm) << "Message received:" << (message.length() > 1000? message.left(1000) : message);
    qWarning() << "OCPP CLIENT: Received Message " << message;
    if (message.length() > 1000)
        qCDebug(OcppComm) << "Note: qDebug message log was truncated at 1000-characters.";
    OcppDataReceived(message);
    resetHeartbeatTriggerCount();
    m_ChargerA.SendUdpLog(message.toLatin1());
}

void OcppClient::resetHeartbeatTriggerCount()
{
    if(!isSendHeaetbeatState)
        heartbeatTriggerCount = 0;
    isSendHeaetbeatState = false;
}

void OcppClient::onSslErrors(const QList<QSslError> &errors)
{
    qCWarning(OcppComm) << "SSL Errors received:";
    for (auto err : errors) {
        qCWarning(OcppComm) << err.errorString();
    }

    // WARNING: Never ignore SSL errors in production code.
    // The proper way to handle self-signed certificates is to add a custom root
    // to the CA store.

    ws.ignoreSslErrors();
}
//! [onTextMessageReceived]

void OcppClient::ConnectServer()
{
    switch (st)
    {
    case State::Idle:
    {
        QString centralServerUrl;
        if(GetSetting(QStringLiteral("[HOST&URL]"), centralServerUrl)) {
            qCDebug(OcppComm) << "Opening connection to " << centralServerUrl;
            st = State::Connecting;
            QNetworkRequest request;
            request.setUrl(QUrl(centralServerUrl));
            request.setRawHeader("Sec-WebSocket-Protocol", QByteArray("ocpp1.6"));
            ws.open(request);
            //ws.open(QUrl(centralServerUrl));
            connTimer.stop();
            connTimer.start(std::chrono::duration_cast<std::chrono::milliseconds>(connMaxWait));
        }
        else {
            qCWarning(OcppComm) << "Unable to find CentralServer URL from Settings File!";
        }
    }
        break;
    case State::Connecting:
        qCWarning(OcppComm) << "ConnectServer: Connect-in-progress, ignore request.";
        break;
    default:
        // Do nothing
        qCWarning(OcppComm) << "ConnectServer: Unexpected OcppClient State (" << static_cast<int>(st) << ") ";
        break;
    }
}

void OcppClient::DisconnectServer()
{
    switch (st){
    case State::Connected:
    case State::CallMessageSent:
    case State::CallResponseRcvd:
        st = State::Disconnecting;
        ws.close();
        break;
    case State::Disconnecting:
        qCWarning(OcppComm) << "DisconnectServer: Disconnect-in-progress, ignore request.";
        break;
    default:
        // Do nothing
        qCWarning(OcppComm) << "DisconnectServer: Unexpected OcppClient State (" << static_cast<int>(st) << ") ";
        break;
    }
}

void OcppClient::CloseWebSocket()
{
    qCDebug(OcppComm) << "Closing WebSocket...";
    st = State::Disconnecting;
    ws.close();
}

void OcppClient::sendTextMessage(QSharedPointer<ocpp::RpcMessage> rpcMsg, bool replace)
{
    // reset heart-beat timer
    //do not reset timer
    //heartBeatTimer.start();
    std::ostringstream oss;
    oss << *rpcMsg;
    QString msgStr = QString::fromStdString(oss.str());
    qWarning() << "OCPP CLIENT: Sent Message " << msgStr;
    if (replace) {
        Q_ASSERT(rpcMsg->getType() == ocpp::RpcMessageType::Call);
        auto callMsg = rpcMsg.dynamicCast<ocpp::CallMessage>();
        ocppMsgOutQ.addReplaceExisting(callMsg);
    } else
        ocppMsgOutQ.add(rpcMsg);
}

//=============================================================================
void  OcppClient::OcppManage()
{
    bool handleRequests = false;

    switch (chargerRegStatus) {
    case ocpp::RegistrationStatus::Unknown:
    case ocpp::RegistrationStatus::Rejected:
    case ocpp::RegistrationStatus::Pending:
        // Do not send any of the requests below
        // Do not respond to requests
        handleRequests = false;
        break;
    case ocpp::RegistrationStatus::Accepted:
        handleRequests = true;
        break;
    }

    if (!handleRequests)
        return;

    //TODO: 增加CALL命令
    //以上是CALL指令

    if(hasPendingInCallRequestsOf(ocpp::CallActionType::UnlockConnector))  //&&(m_CallTimeout))
    { //有没回复的请求:
        EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
        if (reg.hasActiveTransaction()) {
            // Reset the timer if transaction still valid - charging have
            // not been stopped yet. Prevents premature 'UnlockFailed'
            m_ChargerA.LockTimeout = 0;
        }
        if(charger::isConnLockEnabled() && m_ChargerA.IsUnLock())
        {
            QSharedPointer<ocpp::CallMessage> callMsg = removeFirstPendingInCallRequestsOf(ocpp::CallActionType::UnlockConnector);
            ocppReplyUnlockConnector(callMsg->getId().data(), ocpp::UnlockStatus::Unlocked);
        }
        else if(m_ChargerA.LockTimeout >_C_LOCKTIMEOUT)
        {
            QSharedPointer<ocpp::CallMessage> callMsg = removeFirstPendingInCallRequestsOf(ocpp::CallActionType::UnlockConnector);
            ocppReplyUnlockConnector(callMsg->getId().data(), ocpp::UnlockStatus::UnlockFailed);
        }
        //TODO: 增加延时回复指令
    }
}

QString OcppClient::OcppGetItemString(const QJsonObject &json, const ocpp::JsonField key)
{
    auto keyStr = QString(ocpp::jsonFieldStr.at(key).c_str());
    if (json.contains(keyStr) && json[keyStr].isString()) {
        return json[keyStr].toString();
    }
    return QString(); // Null (not empty)
}

int OcppClient::OcppGetItemInt(const QJsonObject &json, const ocpp::JsonField key, int fallbackVal)
{
    auto keyStr = QString(ocpp::jsonFieldStr.at(key).c_str());
#if false // debug-purpose only
    if (!json.contains(keyStr))
        qCWarning(OcppComm) << "does not contain field " << keyStr;
    else if(json[keyStr].isDouble())
        qCWarning(OcppComm) << "value is not double. " << json[keyStr];
#endif
    if (json.contains(keyStr) && json[keyStr].isDouble()) {
        return json[keyStr].toInt();
    }
    return fallbackVal;
}

double OcppClient::OcppGetItemDouble(const QJsonObject &json, const ocpp::JsonField key, double fallbackVal)
{
    auto keyStr = QString(ocpp::jsonFieldStr.at(key).c_str());
    if (json.contains(keyStr) && json[keyStr].isDouble()) {
        return json[keyStr].toDouble();
    }
    return fallbackVal; // Null (not empty)
}

QDateTime OcppClient::OcppGetItemDateTime(const QJsonObject &json, const ocpp::JsonField key)
{
    QDateTime datetime; // null-values
    auto keyStr = QString(ocpp::jsonFieldStr.at(key).c_str());
    if (json.contains(keyStr) && json[keyStr].isString()) {
        QString dtStr = json[keyStr].toString();
        datetime = QDateTime::fromString(dtStr, Qt::DateFormat::ISODateWithMs);
    }
    return datetime;
}

QJsonObject OcppClient::OcppGetItemObj(const QJsonObject &json, const ocpp::JsonField key)
{
    auto keyStr = QString(ocpp::jsonFieldStr.at(key).c_str());
    if (json.contains(keyStr) && json[keyStr].isObject()) {
        return json[keyStr].toObject();
    }
    return QJsonObject(); // Null
}

QJsonArray OcppClient::OcppGetItemArray(const QJsonObject &json, const ocpp::JsonField key)
{
    auto keyStr = QString(ocpp::jsonFieldStr.at(key).c_str());
    if (json.contains(keyStr) && json[keyStr].isArray()) {
        return json[keyStr].toArray();
    }
    return QJsonArray(); // Null
}


std::unique_ptr<ocpp::IdTagInfo> OcppClient::OcppGetIdTagInfo(const QJsonObject &json, const ocpp::JsonField key)
{
    std::unique_ptr<ocpp::IdTagInfo> idTagInfo(nullptr);
    try {
        auto keyStr = QString(ocpp::jsonFieldStr.at(key).c_str());
        if (json.contains(keyStr) && json[keyStr].isObject()) {
            QJsonObject jsonIdTagInfo = json[keyStr].toObject();
            // status (required)
            QString status = OcppGetItemString(jsonIdTagInfo, ocpp::JsonField::status);
            if (status.isNull() || status.isEmpty())
                throw std::runtime_error("AuthorizationStatus field not found");
            std::istringstream iss(status.toStdString());
            struct ocpp::IdTagInfo tmpResult;
            iss >> tmpResult.authStatus;
            // expiryDate (optional)
            QDateTime expDate = OcppGetItemDateTime(jsonIdTagInfo, ocpp::JsonField::expiryDate);
            if (!expDate.isNull()) {
                if (!expDate.isValid())
                    throw std::runtime_error("Expiry Date field is invalid date-time value");
                tmpResult.dateTime = ocpp::DateTime(std::chrono::seconds(expDate.toSecsSinceEpoch()));
            }
            // parentIdTag (optional)
            QString parentId = OcppGetItemString(jsonIdTagInfo, ocpp::JsonField::parentIdTag);
            if (!(parentId.isNull() || parentId.isEmpty())) {
               QByteArray bytes = parentId.toUtf8();
               std::copy(bytes.begin(), bytes.end(), tmpResult.parentId.begin());
            }
            idTagInfo = std::make_unique<ocpp::IdTagInfo>(tmpResult);
        }
        else {
            throw std::runtime_error("Unable to find idTagInfo object");
        }
    }
    catch(std::exception &e) {
        qCWarning(OcppComm) << "idTagInfo Json parse error: " << e.what();
    }
    return idTagInfo;
}


long OcppClient::OcppGetTransactionId(const QByteArray &jsonStr)
{
    long transId = -1;
    try {
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr, &parseErr);
        if (doc.isNull())
            throw std::runtime_error(parseErr.errorString().toStdString());
        QJsonObject obj = doc.object();
        transId = OcppGetItemInt(obj, ocpp::JsonField::transactionId);
        if (transId <= 0)
            throw std::runtime_error("Unable to parse transactionId field");
    }
    catch (std::exception &rerr) {
        qCWarning(OcppComm) << "transactionId field parse error: " << rerr.what();
    }
    return transId;
}

QString OcppClient::ocppGetIdTag(const QByteArray &jsonStr)
{
    QString idTag;
    try {
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr, &parseErr);
        if (doc.isNull())
            throw std::runtime_error(parseErr.errorString().toStdString());
        QJsonObject obj = doc.object();
        idTag = OcppGetItemString(obj, ocpp::JsonField::idTag);
        if (idTag.isNull() || idTag.isEmpty())
            throw std::runtime_error("Unable to parse transactionId field");
    }
    catch (std::exception &rerr) {
        qCWarning(OcppComm) << "idTag field parse error: " << rerr.what();
    }
    return idTag;
}

void OcppClient::OcppDataReceived(QString& message)
{
    // Try decode the message
    std::shared_ptr<ocpp::RpcMessage> rpcMsg = ocpp::CallMessage::fromMessageString(message.toStdString());
    if (!rpcMsg) {
        rpcMsg = ocpp::CallResultMessage::fromMessageString(message.toStdString());
    }
    if (!rpcMsg) {
        rpcMsg = ocpp::CallErrorMessage::fromMessageString(message.toStdString());
    }

    if(rpcMsg)
    {
        QString uniqueId = rpcMsg->getId().c_str();

        bool handleCallAction = false, handleCallResults = false;
        switch(chargerRegStatus) {
        case ocpp::RegistrationStatus::Unknown:
        case ocpp::RegistrationStatus::Rejected:
            handleCallAction = false;
            handleCallResults = true;
            break;
        case ocpp::RegistrationStatus::Accepted:
        case ocpp::RegistrationStatus::Pending:
            handleCallAction = true;
            handleCallResults = true;
            break;
        }

        switch(rpcMsg->getType())
        {
        case ocpp::RpcMessageType::Call:     //HOST CALL
            if (handleCallAction)
                handleCallMessage(std::dynamic_pointer_cast<ocpp::CallMessage>(rpcMsg));
            break;

        case  ocpp::RpcMessageType::CallResult:     //CALLRESULT 平台对桩命令的响应
        {
            QSharedPointer<ocpp::RpcMessage> matchingRpcMsg = ocppMsgOutQ.removeSentCallMessage(rpcMsg->getId().data());
            if (!matchingRpcMsg.isNull()) {
                if (st == State::CallMessageSent) {
                    st = State::CallResponseRcvd;
                    if (handleCallResults) {
                        Q_ASSERT(matchingRpcMsg->getType() == ocpp::RpcMessageType::Call);
                        handleCallResultMessage(
                                    std::dynamic_pointer_cast<ocpp::CallResultMessage>(rpcMsg),
                                    matchingRpcMsg.dynamicCast<ocpp::CallMessage>());
                    }
                }
                else {
                    qCWarning(OcppComm) << "OcppClient: Unexpected state "
                            << static_cast<int>(st) << " receiving CallResult.";
                }
            }
            else {
                qCWarning(OcppComm) << "OcppClient: Unable to find corresponding Call Message Id:" << rpcMsg->getId().data();
            }
        }
            break;
        case  ocpp::RpcMessageType::CallError:     //CALLERROR
        {
            QSharedPointer<ocpp::RpcMessage> matchingRpcMsg = ocppMsgOutQ.removeSentCallMessage(rpcMsg->getId().data());
            if (!matchingRpcMsg.isNull()) {                if (st == State::CallMessageSent) {
                    st = State::CallResponseRcvd;
                    if (handleCallResults)
                        handleCallErrorMessage(std::dynamic_pointer_cast<ocpp::CallErrorMessage>(rpcMsg),
                                               matchingRpcMsg.dynamicCast<ocpp::CallMessage>());
                }
                else {
                    qCWarning(OcppComm) << "OcppClient: Unexpected state "
                            << static_cast<int>(st) << " receiving CallResult.";
                }
            }
            else {
                qCWarning(OcppComm) << "OcppClient: Unable to find corresponding Call Message Id:" << rpcMsg->getId().data();
            }
         }
            break;
        default:
            qCWarning(OcppComm) <<"OCPP MessageType error";
            break;
        }
    }
    else
    {
        qCWarning(OcppComm) << "Unable to decode the RPC-Message, possibly malformed message";
    }
}

void OcppClient::handleCallMessage(std::shared_ptr<ocpp::CallMessage> callMsg)
{
    QString uniqueId = callMsg->getId().data();
    ocpp::CallActionType callAction = ocpp::CallActionType::Nil;
    std::istringstream iss(callMsg->getAction());
    iss >> callAction; // String to Enum
    QJsonParseError payloadParseErr;
    QJsonDocument payloadDoc = QJsonDocument::fromJson(callMsg->getPayload().c_str(), &payloadParseErr);
    if (payloadDoc.isNull()) {
        throw std::runtime_error(payloadParseErr.errorString().toStdString());
    }
    const QJsonObject payloadObj = payloadDoc.object();
    switch (callAction) {
    case ocpp::CallActionType::GetLocalListVersion:
    {
        ocpp::LocalAuthorizationListVersion listVer = getLocalAuthListVer();
        ocppReplyGetLocalListVersion(uniqueId, listVer);
#if false
        // Borrow this trigger UpdateFirmware
        //QString uri = "scp://evmega@192.168.1.102/release_1_2_3/LibreOffice_6.3.5_Win_x64.msi";
        //QString uri = "scp://evmega@192.168.1.102/a.zip";
        QString uri = "sftp://h2.k-solution.com.hk/firmwares/675acde0-7e02-11ea-81b1-69b3c289edb0";
        FirmwareUpdateScheduler &fwupdate = FirmwareUpdateScheduler::getInstance();
        QDateTime availTime = QDateTime::currentDateTime().addSecs(30);
        fwupdate.newUpdateRequest(uri, availTime, 5, 15);
#endif
    }
        break;
    case ocpp::CallActionType::SendLocalList:
    {
        // Check that all required fields are present
        ocpp::UpdateStatus confStatus = ocpp::UpdateStatus::Failed;
        try {
            ocpp::LocalAuthorizationListVersion nextListVer = OcppGetItemInt(payloadObj, ocpp::JsonField::listVersion);
            QString updateType = OcppGetItemString(payloadObj, ocpp::JsonField::updateType);
            if (updateType.isNull() || updateType.isEmpty()) {
                throw ocpp::UpdateStatus::Failed;
            }
            ocpp::LocalAuthorizationListVersion curListVer = getLocalAuthListVer();
            // Handle Full and Differential Types differently
            bool listUpdated = false;
            if (updateType == "Differential") {
                // Must be a newer version
                if (nextListVer <= curListVer)
                    throw ocpp::UpdateStatus::VersionMismatch;
                 QJsonArray authItems = OcppGetItemArray(payloadObj, ocpp::JsonField::localAuthorizationList);
                // Update LocalAuthList by UID one-by-one
                 std::vector<ocpp::AuthorizationData> diffListData = jsonArrayToAuthData(authItems);
                 LocalAuthList &authList = LocalAuthList::getInstance();
                 listUpdated = authList.update(diffListData);
                 if (!listUpdated)
                     throw std::runtime_error("Update items failed.");
                 else
                     qCDebug(OcppComm()) << "SendLocalList Differential update - items updated" << endl;
            }
            else if (updateType == "Full") {
                // Only make changes if the version is different
                if (nextListVer != curListVer) {
                    QJsonArray authItems = OcppGetItemArray(payloadObj, ocpp::JsonField::localAuthorizationList);
                    LocalAuthList &authList = LocalAuthList::getInstance();
                    if (authItems.isEmpty()) {
                        // Remove all Items
                        listUpdated = authList.removeAll();
                        if (!listUpdated) {
                            throw std::runtime_error("Clear all items failed.");
                        }
                        else {
                            qCDebug(OcppComm()) << "SendLocalList Full update - all items deleted" << endl;
                        }
                    }
                    else {
                        // Convert Json object into AuthorizationData
                        std::vector<ocpp::AuthorizationData> fullListData = jsonArrayToAuthData(authItems);
                        LocalAuthList &authList = LocalAuthList::getInstance();
                        listUpdated = authList.replaceAll(fullListData);
                        if (!listUpdated)
                            throw std::runtime_error("Replace items failed.");
                        else
                            qCDebug(OcppComm()) << "SendLocalList Full update - items updated" << endl;
                    }
                }
                else {
                    qCDebug(OcppComm()) << "SendLocalList Full update action not taken because there is no change in List-Version";
                }
            }
            // Update LocalAuthList version
            if (listUpdated) {
                 LocalAuthList &authList = LocalAuthList::getInstance();
                 if (!authList.setVersion(nextListVer)) {
                     throw std::runtime_error("Error updating List-Version to database.");
                 }
                 confStatus = ocpp::UpdateStatus::Accepted;
            }
        }
        catch (ocpp::UpdateStatus errUpdateStatus) {
            confStatus = errUpdateStatus;
        }
        catch (std::runtime_error &re) {
            qCWarning(OcppComm) << callAction << ": " << re.what();
            confStatus = ocpp::UpdateStatus::Failed;
        }
        ocppReplySendLocalList(uniqueId, confStatus);
    }
        break;
    case ocpp::CallActionType::RemoteStartTransaction:
    {
        // capture the parameters first
        QString idTag = OcppGetItemString(payloadObj, ocpp::JsonField::idTag);
        unsigned long chargeTimeVal = m_ChargerA.ChargeToFullTimeSecs;
        double currentLimit = -1.0;

        QJsonObject chargingProfile = OcppGetItemObj(payloadObj, ocpp::JsonField::chargingProfile);
        if (!chargingProfile.isEmpty()) {
            QJsonObject chargingSchedule = OcppGetItemObj(chargingProfile, ocpp::JsonField::chargingSchedule);
//            if (!chargingSchedule.isEmpty()) {
//                int duration = OcppGetItemInt(chargingSchedule, ocpp::JsonField::duration);
//                if (duration > 0) // do not overeride default '60' by mistake
//                    chargeTimeVal = duration;
//            }
            QJsonArray chargingSchedulePeriods = OcppGetItemArray(chargingSchedule, ocpp::JsonField::chargingSchedulePeriod);
            if (!chargingSchedulePeriods.isEmpty()) {
                qCWarning(OcppComm) << "Only handle the first ChargingSchedulePeriod for now.";
                if (chargingSchedulePeriods[0].isObject()) {
                    currentLimit = OcppGetItemDouble(chargingSchedulePeriods[0].toObject(), ocpp::JsonField::limit);
                }
            }
        }
        else
        {
            chargeTimeVal = m_ChargerA.ChargeToFullTimeSecs;
            currentLimit = m_ChargerA.GetCurrentLimit();
        }
        remoteStartTxAction.schedule(idTag, chargeTimeVal, currentLimit);
        if(m_ChargerA.IsPreparing()
            ||(m_ChargerA.IsBookingState() &&(idTag == m_ChargerA.ResIdTag) &&(m_ChargerA.IsPreparingState()
                ||(m_ChargerA.ConnType() == CCharger::ConnectorType::WallSocket)))
            ||((m_ChargerA.ConnType() == CCharger::ConnectorType::WallSocket)&&m_ChargerA.IsIdleState())
            && (m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected()))
        {
            remoteStartTxAction.go();
            ocppReplyRemoteStartTransaction(uniqueId, ocpp::RemoteStartStopStatus::Accepted);
        }
        else if (m_ChargerA.IsFinishing()) {
            // Unlock Connector
            if (charger::isConnLockEnabled() && !m_ChargerA.IsUnLock())
                m_ChargerA.UnLock();
            // Schedule Start Charging once the charger state goes to Idle
            QTimer::singleShot(1000, &remoteStartTxAction, &RmtStartTxExec::go);
            ocppReplyRemoteStartTransaction(uniqueId, ocpp::RemoteStartStopStatus::Accepted);
        }
        else
        {
            ocppReplyRemoteStartTransaction(uniqueId, ocpp::RemoteStartStopStatus::Rejected);
        }
    }
        break;
    case ocpp::CallActionType::RemoteStopTransaction:
    {
        EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
        int transId = OcppGetItemInt(payloadObj, ocpp::JsonField::transactionId, -1);
        if ((transId >= 0) && reg.isActiveTransactionId(QString::number(transId))
           && m_ChargerA.IsChargingOrSuspended())
        {
            m_ChargerA.StopCharge();
            m_ChargerA.StopReason = ocpp::Reason::Remote;

            ocppReplyRemoteStopTransaction(uniqueId, ocpp::RemoteStartStopStatus::Accepted);

            m_ChargerA.UnLock();    //远程停止充电时同时开锁
            emit checkunlock();
        }
        else
        {
            if (m_ChargerA.IsFinishing()) {
                // Unlock if Finishing - recommended by Fung
                m_ChargerA.UnLock();    //远程停止充电时同时开锁
                emit checkunlock();
            }
            ocppReplyRemoteStopTransaction(uniqueId, ocpp::RemoteStartStopStatus::Rejected);
        }
    }
        break;
    case ocpp::CallActionType::UnlockConnector:
    {
        if (charger::isConnLockEnabled()) {
            QSharedPointer<ocpp::CallMessage> savedMsg = QSharedPointer<ocpp::CallMessage>::create(callMsg->getId(), callMsg->getAction(), callMsg->getPayload());
            addToPendingInCallRequests(savedMsg); // save for later Call results/error
            // Stop Charging first if it is on-going
            if (m_ChargerA.IsChargingOrSuspended() || m_ChargerA.IsSuspendedStartEVSE()) {
                m_ChargerA.StopCharge();
                m_ChargerA.StopReason = ocpp::Reason::UnlockCommand;
                m_ChargerA.UnLock();
                emit checkunlock();
            }
            else {
                // Force Unlock-Connector, regardless of current status
                 m_ChargerA.UnLock();
                 emit checkunlock();
            }
        }
        else {
            ocppReplyUnlockConnector(uniqueId, ocpp::UnlockStatus::NotSupported);
        }
    }
        break;
    case ocpp::CallActionType::ClearCache:
    {
        if (authCache.removeAll()) {
            authCacheFlushTimer.start();
            ocppReplyClearCache(uniqueId, ocpp::ClearCacheStatus::Accepted);
        }
        else {
            ocppReplyClearCache(uniqueId, ocpp::ClearCacheStatus::Rejected);
        }
    }
        break;
    case ocpp::CallActionType::ChangeAvailability:
    {
        QString typeFieldStr = OcppGetItemString(payloadObj, ocpp::JsonField::type);
        ocpp::AvailabilityStatus status;
        try {
            if (typeFieldStr.isNull() || typeFieldStr.isEmpty()) {
                throw std::runtime_error("ChangeAvailability.req missing 'type' field");
            }
            ocpp::AvailabilityType availType;
            std::istringstream iss(typeFieldStr.toStdString());
            iss >> availType;
            if (availType == ocpp::AvailabilityType::Operative && connUnavailPending) {
                connUnavailPending = false;
                emit cancelScheduleConnUnavailChange();
                status = ocpp::AvailabilityStatus::Accepted;
            }
            else {
                switch(m_ChargerA.ChangeConnAvail(availType == ocpp::AvailabilityType::Operative)){
                case CCharger::Response::OK:
                    status = ocpp::AvailabilityStatus::Accepted;
                    break;
                case CCharger::Response::Busy:
                    status = ocpp::AvailabilityStatus::Scheduled;
                    connUnavailPending = true;
                    emit scheduleConnUnavailChange();
                    break;
                case CCharger::Response::Denied:
                    throw std::runtime_error("ChangeAvailability request rejected.");
                    break;
                }
            }
        }
        catch(std::exception &e) {
            qCWarning(OcppComm) << "ChangeAvailability: " << e.what();
            status = ocpp::AvailabilityStatus::Rejected;
        }
        ocppReplyChangeAvailability(uniqueId, status);
    }
        break;
    case ocpp::CallActionType::ChangeConfiguration:
    {
        QString txt = OcppGetItemString(payloadObj, ocpp::JsonField::key);
        int value = OcppGetItemInt(payloadObj, ocpp::JsonField::value);;

        if(txt =="ConnectionTimeOut")
        {
            if(value >0) m_CableTimeoutKeyValue =value;
        }
        else if(txt =="MeterValueSampleInterval")
        {
            if(value > 0) {
                meterIntervalKeyValue = std::chrono::seconds(value);
                emit meterSampleIntervalCfgChanged();
            }
        }
        else if(txt =="AuthorizeRemoteTxRequests")
        {
            if(value > 0) {
            }
        }
        else if(txt =="ClockAlignedDataInterval")
        {
            if(value > 0) {
            }
        }
        else if(txt =="HeartbeatInterval")
        {
            if(value > 0) {
            }
        }
        else if(txt =="LocalAuthorizeOffline")
        {
            if(value > 0) {
            }
        }
        else if(txt =="LocalPreAuthorize")
        {
            if(value > 0) {
            }
        }
        else if(txt =="MeterValuesAlignedData")
        {
            if(value > 0) {
            }
        }
        else if(txt =="MeterValuesSampledData")
        {
            if(value > 0) {
            }
        }
        else if(txt =="ResetRetries")
        {
            if(value > 0) {
            }
        }
        else if(txt =="ConnectorPhaseRotation")
        {
            if(value > 0) {
            }
        }
        else if(txt =="StopTransactionOnEVSideDisconnect")
        {
            if(value > 0) {
            }
        }
        else if(txt =="StopTransactionOnInvalidId")
        {
            if(value > 0) {
            }
        }
        else if(txt =="StopTxnAlignedData")
        {
            if(value > 0) {
            }
        }
        else if(txt =="StopTxnSampledData")
        {
            if(value > 0) {
            }
        }
        else if(txt =="TransactionMessageAttempts")
        {
            if(value > 0) {
            }
        }        
        else if(txt =="TransactionMessageRetryInterval")
        {
            if(value > 0) {
            }
        }
        else if(txt =="UnlockConnectorOnEVSideDisconnect")
        {
            if(value > 0) {
            }
        }

        ocppReplyChangeConfiguration(uniqueId, ocpp::ConfigurationStatus::Accepted);
    }
        break;
    case ocpp::CallActionType::Reset:
    {
        QString txt = OcppGetItemString(payloadObj, ocpp::JsonField::type);

        chargerRegStatus = ocpp::RegistrationStatus::Unknown;

        if(txt =="Hard")
        {
            ocppReplyReset(uniqueId, ocpp::ResetStatus::Accepted);
            emit hardreset();
            //TODO:
        }
        else if(txt =="Soft")
        {
            ocppReplyReset(uniqueId, ocpp::ResetStatus::Accepted);
            emit hardreset();
            //TODO
        }
        else
        {
            ocppReplyReset(uniqueId, ocpp::ResetStatus::Rejected);
        }

  }
        break;
    case ocpp::CallActionType::GetConfiguration:
    {
        ocpp::KeyValue kv(ocpp::configurationKeyStrs.at(ocpp::ConfigurationKeys::MeterValueSampleInterval).data(),
                          QString::number(meterIntervalKeyValue.count()), true);
        std::vector<ocpp::KeyValue> cfgs{kv};
        ocppReplyGetConfiguration(uniqueId, cfgs);
    }
        break;
    case ocpp::CallActionType::ReserveNow:
    {
        QDateTime expiryDate = OcppGetItemDateTime(payloadObj, ocpp::JsonField::expiryDate);
#if true
        int rsnId = OcppGetItemInt(payloadObj, ocpp::JsonField::reservationId);
#else
        // Test Platform bug - reservationId is sent as String instead of Integer(Double)
        int rsnId = OcppGetItemString(payloadObj, ocpp::JsonField::reservationId).toInt();
#endif
        QString idTag = OcppGetItemString(payloadObj, ocpp::JsonField::idTag);

        //qCDebug(OcppComm) <<"BOOKING " <<expiryDate;
        //TODO: 分不同的枪
        ocpp::ReservationStatus resStatus = ocpp::ReservationStatus::Rejected;
        if(m_ChargerA.IsCablePlugIn() || m_ChargerA.IsEVConnected() || idTag.isNull() || idTag.isEmpty() || (rsnId <= 0) || !(expiryDate.isValid() && (expiryDate>QDateTime::currentDateTime())))
        {
            resStatus = ocpp::ReservationStatus::Rejected;
        }
        else if((rsnId == m_ChargerA.ReservationId) && m_ChargerA.IsBookingState())
        {
            m_ChargerA.ResIdTag     = idTag;
            m_ChargerA.ExpiryDate   = expiryDate;
            resStatus = ocpp::ReservationStatus::Accepted;
        }
        else if(m_ChargerA.IsFaultState())
        {
            resStatus = ocpp::ReservationStatus::Faulted;
        }
        else if(!m_ChargerA.IsConnectorAvailable())
        {
            resStatus = ocpp::ReservationStatus::Unavailable;

        }
        else if(m_ChargerA.IsIdleState())
        {
            m_ChargerA.ReservationId = rsnId;
            m_ChargerA.ResIdTag     = idTag;
            m_ChargerA.ExpiryDate   = expiryDate;
            m_ChargerA.SetBooking();
            resStatus = ocpp::ReservationStatus::Accepted;
        }
        else
        {
            resStatus = ocpp::ReservationStatus::Occupied;
        }

        ocppReplyReserveNow(uniqueId, resStatus);
    }
        break;
    case ocpp::CallActionType::CancelReservation:
    {
#if true
        int rsnId =OcppGetItemInt(payloadObj, ocpp::JsonField::reservationId);
#else
        // Test Platform bug - reservationId sent as String instead of Integer expected.
        int rsnId = OcppGetItemString(payloadObj, ocpp::JsonField::reservationId).toInt();
#endif
        QString outMsgStr;
        if(rsnId == m_ChargerA.ReservationId)
        {
            if(m_ChargerA.IsBookingState())
            {
                m_ChargerA.ChargerIdle();
            }
            ocppReplyCancelReservation(uniqueId, ocpp::CancelReservationStatus::Accepted);
        }
        else {
            ocppReplyCancelReservation(uniqueId, ocpp::CancelReservationStatus::Rejected);
        }
    }
        break;
    case ocpp::CallActionType::SetChargingProfile:
    {
        //connectorId
        //SetChargingProfile -> csChargingProfiles -> chargingSchedule -> [chargingSchedulePeriod >=1 ]
        ocpp::ChargingProfileStatus setProfileStatus{ocpp::ChargingProfileStatus::Rejected};
        try {
            QJsonObject csChargingProfiles = OcppGetItemObj(payloadObj, ocpp::JsonField::csChargingProfiles);
            if (csChargingProfiles.isEmpty())
                throw std::runtime_error("SetChargingProfile.csChargingProfiles cannot be parsed.");
            QJsonObject chargingSchedule = OcppGetItemObj(csChargingProfiles, ocpp::JsonField::chargingSchedule);
            if (chargingSchedule.isEmpty())
                throw std::runtime_error("SetChargingProfile.csChargingProfiles.chargingSchedule cannot be parsed.");
            QJsonArray chargingSchedulePeriods = OcppGetItemArray(chargingSchedule, ocpp::JsonField::chargingSchedulePeriod);
            if (chargingSchedulePeriods.isEmpty())
                throw std::runtime_error("SetChargingProfile.csChargingProfiles.chargingSchedule.chargingSchedulePeriod array cannot be parsed.");
            // obtain the limit for the first schedule
            qCWarning(OcppComm) << "Only handle first charge-schedule-period element for now";
            if (!chargingSchedulePeriods[0].isObject())
                throw std::runtime_error("SetChargingProfile.csChargingProfiles.chargingSchedule.chargingSchedulePeriod array element is not an Json Object Type.");
            double limit = OcppGetItemDouble(chargingSchedulePeriods[0].toObject(), ocpp::JsonField::limit, -1.0);
            if (limit >= 0) {
                QString purposeStr = OcppGetItemString(csChargingProfiles, ocpp::JsonField::chargingProfilePurpose);
                std::istringstream iss(purposeStr.toStdString());
                ocpp::ChargingProfilePurposeType purpose;
                iss >> purpose;
#if false
                // override for development
                if (static_cast<int>(limit) == 0)
                    purpose = ocpp::ChargingProfilePurposeType::TxDefaultProfile;
#endif
                switch (purpose) {
                case ocpp::ChargingProfilePurposeType::TxDefaultProfile:
                    m_ChargerA.SetDefaultProfileCurrent(static_cast<int>(limit));
                    break;
                default:
                    m_ChargerA.SetProfileCurrent(static_cast<int>(limit));
                    break;
                }
                qCDebug(OcppComm) <<"***SetChargingProfile:***" << " Limit=" << static_cast<int>(limit);
                setProfileStatus = ocpp::ChargingProfileStatus::Accepted;
            }
            else {
                throw std::runtime_error("Unexpected charging limit value of 0 or malformed charging limit Json string");
            }
        }
        catch (std::runtime_error &rerr) {
            qCWarning(OcppComm) << rerr.what();
            setProfileStatus = ocpp::ChargingProfileStatus::Rejected;
        }
        ocppReplySetChargingStatus(uniqueId, setProfileStatus);
    }
        break;
    case ocpp::CallActionType::UpdateFirmware:
    {
        QString location = OcppGetItemString(payloadObj, ocpp::JsonField::location);
        // location must not be empty
        if (location.isNull() || location.isEmpty()) {
            throw std::runtime_error("UpdateFirmware.location is required and must not be empty");
        }
        QDateTime scheduledStart = OcppGetItemDateTime(payloadObj, ocpp::JsonField::retrieveDate);
        if (scheduledStart.isNull() || !scheduledStart.isValid()) {
            throw std::runtime_error("UpdateFirmware.retrieveDate is required and must not be empty");
        }

        int numRetries = OcppGetItemInt(payloadObj, ocpp::JsonField::retries);
        if (numRetries == 0) {
            // not given
            qCWarning(OcppComm) << "UpdateFirmware.req::retries it not given, using default value";
            numRetries = -1;
        }
        int retryIntervalSecs = OcppGetItemInt(payloadObj, ocpp::JsonField::retryInterval);
        if (retryIntervalSecs == 0) {
            qCWarning(OcppComm) << "UpdateFirmware.req::retryInterval it not given, using default value";
            retryIntervalSecs = -1;
        }
        FirmwareUpdateScheduler &fwupdate = FirmwareUpdateScheduler::getInstance();
        fwupdate.newUpdateRequest(location, scheduledStart, numRetries, retryIntervalSecs);

        // There is no fields defined UpdateFirmware.conf
        ocppReplyUpdateFirmware(uniqueId, ocpp::NotdefinedStatus::Accepted);
    }
        break;
    case ocpp::CallActionType::TriggerMessage:
    {
        QString requestMsg = OcppGetItemString(payloadObj, ocpp::JsonField::requestMessage);
        if(requestMsg == ocpp::callActionStrs.at(ocpp::CallActionType::BootNotification).data())
        {
            ocppReplyTriggerMessage(uniqueId, ocpp::TriggerMessageStatus::Accepted);
            ocppSendBootNotification();
        }
        else if(requestMsg == ocpp::callActionStrs.at(ocpp::CallActionType::StatusNotification).data())
        {
            ocppReplyTriggerMessage(uniqueId, ocpp::TriggerMessageStatus::Accepted);
            addStatusNotificationReq();
        }else if(requestMsg == ocpp::callActionStrs.at(ocpp::CallActionType::DistanceSensing).data())
        {
            ocppReplyTriggerMessage(uniqueId, ocpp::TriggerMessageStatus::Accepted);
            ocppParkingDataTransfer(
                        (DistanceSensorThread::GetDistance()*100 < charger::GetDistThreshold())?"occupied":"free"
                        ,DistanceSensorThread::GetDistance());
        }
//            case ocpp::CallActionType::FirmwareStatusNotification:
//            {
//                ocppReplyTriggerMessage(uniqueId, ocpp::TriggerMessageStatus::Accepted);
//                ocppSendFirmwareStatusNotification();
//            }
        else
        {
            ocppReplyTriggerMessage(uniqueId, ocpp::TriggerMessageStatus::NotImplemented);
        }


    }
        break;
    case ocpp::CallActionType::DataTransferTimeValue:
    {
        QString messageId = OcppGetItemString(payloadObj, ocpp::JsonField::messageId);

        if(messageId != NULL)
        {
            if(messageId == "popupMessage")
            {
                QString popUpMsgDataString = OcppGetItemString(payloadObj, ocpp::JsonField::data);
                QByteArray br = popUpMsgDataString.toUtf8();
                QJsonDocument doc = QJsonDocument::fromJson(br);
                QJsonObject popUpMsgData = doc.object();
                //QJsonObject popUpMsgData = OcppGetItemObj(payloadObj, ocpp::JsonField::data);
                if(OcppGetItemString(popUpMsgData, ocpp::JsonField::popUpMsgType) == "displayMessage")
                {
                    QJsonObject titleJSON = OcppGetItemObj(popUpMsgData, ocpp::JsonField::title);
                    m_ChargerA.popupMsgTitlezh = OcppGetItemString(titleJSON, ocpp::JsonField::zhhk);
                    m_ChargerA.popupMsgTitleen = OcppGetItemString(titleJSON, ocpp::JsonField::en);
                    QJsonObject contentJSON = OcppGetItemObj(popUpMsgData, ocpp::JsonField::content);
                    m_ChargerA.popupMsgContentzh = OcppGetItemString(contentJSON, ocpp::JsonField::zhhk);
                    m_ChargerA.popupMsgContenten = OcppGetItemString(contentJSON, ocpp::JsonField::en);
                    m_ChargerA.popupMsgDuration = OcppGetItemInt(popUpMsgData, ocpp::JsonField::duration);
                    m_ChargerA.popupMsgContentFontSize = OcppGetItemInt(popUpMsgData, ocpp::JsonField::fontsize);
                    m_ChargerA.popupMsgContentColor = OcppGetItemString(popUpMsgData, ocpp::JsonField::contentColor);
                    m_ChargerA.popupMsgTitleColor = OcppGetItemString(popUpMsgData, ocpp::JsonField::titleColor);
                    m_ChargerA.popupIsQueueingMode = OcppGetItemInt(popUpMsgData, ocpp::JsonField::exit);

                    emit popUpMsgReceived();
                    ocppReplyDataTransfer(uniqueId, ocpp::DataTransferStatus::Accepted);
                }
            }
            else if(messageId == "cancelPopupMessage")
            {
                emit cancelPopUpMsgReceived();
                ocppReplyDataTransfer(uniqueId, ocpp::DataTransferStatus::Rejected);
            }
            else if(messageId == "changeLedLight")
            {
                QString popUpMsgDataString = OcppGetItemString(payloadObj, ocpp::JsonField::data);
                QByteArray br = popUpMsgDataString.toUtf8();
                QJsonDocument doc = QJsonDocument::fromJson(br);
                QJsonObject changeLedLightData = doc.object();
                //QJsonObject changeLedLightData = OcppGetItemObj(payloadObj, ocpp::JsonField::data);
                int ledColor = OcppGetItemInt(changeLedLightData, ocpp::JsonField::color);
                if(ledColor >= 0 && ledColor <= 7)
                {
                    emit changeLedLight(ledColor);
                    ocppReplyDataTransfer(uniqueId, ocpp::DataTransferStatus::Accepted);
                }
                else
                {
                    ocppReplyDataTransfer(uniqueId, ocpp::DataTransferStatus::Rejected);
                }
            }
            else
            {
                ocppReplyDataTransfer(uniqueId, ocpp::DataTransferStatus::NotImplemented);
            }
        }
        else
        {
            ocppReplyDataTransfer(uniqueId, ocpp::DataTransferStatus::NotImplemented);
        }
    }
        qWarning("replied pop up msg");
        break;
    default:
        //TODO: ADD do with other req
        qCWarning(OcppComm) << "Unhandled Ocpp Request: " << callAction;
        break;
    }

}

void OcppClient::handleCallResultMessage(std::shared_ptr<ocpp::CallResultMessage> msg,
                                         QSharedPointer<ocpp::CallMessage> callMsg)
{
    QJsonParseError payloadParseErr;
    QJsonDocument payloadDoc = QJsonDocument::fromJson(msg->getPayload().c_str(), &payloadParseErr);
    if (payloadDoc.isNull())
        throw std::runtime_error(payloadParseErr.errorString().toStdString());
    QJsonObject payloadObj = payloadDoc.object();
    std::istringstream iss(callMsg->getAction());
    ocpp::CallActionType callAction;
    iss >> callAction;
    switch(callAction) {
    case ocpp::CallActionType::BootNotification:
    {
        QString regStatus = OcppGetItemString(payloadObj, ocpp::JsonField::status);
        int interval = OcppGetItemInt(payloadObj, ocpp::JsonField::interval);
        if (interval == 0)
            qCWarning(OcppComm) << "Unable to parse BootNotification.conf.interval field";
        if (regStatus.isNull() || regStatus.isEmpty()) {
            qCWarning(OcppComm) << "Uable to parse BootNotification.conf.status field";
            ocppConnectionStatus = false;
        }
        else {
            if (regStatus == "Accepted") {
                chargerRegStatus = ocpp::RegistrationStatus::Accepted;
                QDateTime currentTime = OcppGetItemDateTime(payloadObj, ocpp::JsonField::currentTime);
                if (currentTime.isNull() || !currentTime.isValid())
                    qCWarning(OcppComm) << "Unable to parse BootNotification.conf.currentTime";
                else
                    emit serverDateTimeReceived(currentTime);
                if(interval > 0) {
                    hbTimerInterval = std::chrono::seconds(interval);
                    heartBeatTimer.setInterval(hbTimerInterval);
                    heartBeatTimer.start();
                    heartbeatTriggerCountTimer.start();
                }
                qCDebug(OcppComm) << "hbTimerInterval: " << hbTimerInterval.count() << " ms";
                ocppConnectionStatus = true;
                emit connStatusOnlineIndicator();
                emit bootNotificationAccepted();
            }
            else {
                ocppConnectionStatus = false;
                qCWarning(OcppComm) << "BootNotification.conf status ("
                              << regStatus << ") is not handled: ";
                // Set up timer with 'interval' value to retry
                if (interval <= 0)
                    bootNotifTimer.setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(defaultBootNotifRetryInterval));
                else
                    bootNotifTimer.setInterval(interval * 1000);
                bootNotifTimer.start();
            }
        }
    }
        break;
    case ocpp::CallActionType::Authorize:
    {
        ocpp::AuthorizationStatus authStatus = ocpp::AuthorizationStatus::Invalid;
        bool authStatusSet = false;
        try {
            std::unique_ptr<ocpp::IdTagInfo> idTagInfo = OcppGetIdTagInfo(payloadObj, ocpp::JsonField::idTagInfo);
            if (!idTagInfo)
                throw std::runtime_error("Unable to parse idTagInfo");
            authStatus = idTagInfo->authStatus;
            authStatusSet = true;
            authCache.addEntry(m_ChargerA.IdTag, *idTagInfo);
            authCacheFlushTimer.start();
            switch (authStatus) {
            case ocpp::AuthorizationStatus::Accepted:
                break;
            default:
                std::ostringstream oss;
                oss << authStatus;
                throw std::runtime_error("AuthorizationStatus is not Accepted: " + oss.str());
            }
        }
        catch (std::runtime_error &rerr) {
            qCWarning(OcppComm) << "Authorize.conf: " << rerr.what();
        }
        emit OcppAuthorizeResponse(authStatusSet, authStatus);

        ocppConnectionStatus = true;
        emit connStatusOnlineIndicator();

    }
        break;
    case ocpp::CallActionType::Heartbeat:
    {
        QDateTime currentTime = OcppGetItemDateTime(payloadObj, ocpp::JsonField::currentTime);
        if (currentTime.isNull() || !currentTime.isValid())
            qCWarning(OcppComm) << "Unable to parse Heartbeat.conf.currentTime";
        else
            emit serverDateTimeReceived(currentTime);
        
        connectionFailTimer.stop();

        ocppConnectionStatus = true;
        emit connStatusOnlineIndicator();
    }
        break;
    case ocpp::CallActionType::StartTransaction:
        try {
            std::unique_ptr<ocpp::IdTagInfo> idTagInfo = OcppGetIdTagInfo(payloadObj, ocpp::JsonField::idTagInfo);
            if (!idTagInfo)
                throw std::runtime_error("Unable to parse idTagInfo field");
            // IdTag can be found from StartTransction.req
            QString idTag = ocppGetIdTag(callMsg->getPayload().c_str());
            authCache.updateEntry(idTag, *idTagInfo);
            authCacheFlushTimer.start();
            if (idTagInfo->authStatus == ocpp::AuthorizationStatus::Accepted
                    || idTagInfo->authStatus == ocpp::AuthorizationStatus::ConcurrentTx) {
                int transId = OcppGetItemInt(payloadObj, ocpp::JsonField::transactionId);
                if (transId <= 0)
                    throw std::runtime_error("Unable to parse transactionId field");
                emit startTransactionResponse(idTagInfo->authStatus, idTag, QString::number(transId));
                qCDebug(OcppComm) << "StartTransaction Accepted: tid: " << transId;
            }
            else {
                int transId = OcppGetItemInt(payloadObj, ocpp::JsonField::transactionId);
                if (transId <= 0)
                    throw std::runtime_error("Unable to parse transactionId field");
                emit startTransactionResponse(idTagInfo->authStatus, idTag, QString::number(transId));
                m_ChargerA.StopCharge();
                m_ChargerA.StopReason = ocpp::Reason::DeAuthorized;

                m_ChargerA.UnLock();    //远程停止充电时同时开锁
                emit checkunlock();
            }

            ocppConnectionStatus = true;
            emit connStatusOnlineIndicator();

        }
        catch (std::runtime_error &rerr) {
            qCWarning(OcppComm) << "StartTransaction.conf: " << rerr.what();
            // Do not stop charge because this might not correspond to what is happening now.
        }
        break;
    case ocpp::CallActionType::MeterValues:
    {
        try {
            int callMsgTransId = OcppGetTransactionId(callMsg->getPayload().c_str());
            if (callMsgTransId <= 0)
                throw std::runtime_error("Unable to parse transactionId field");
            emit meterValueResponse(QString::number(callMsgTransId));
        }
        catch (std::exception &rerr) {
            qCWarning(OcppComm) << "MeterValues.conf: " << rerr.what();

        }

        ocppConnectionStatus = true;
        emit connStatusOnlineIndicator();
    }
        break;
    case ocpp::CallActionType::StopTransaction:
        try {
            std::unique_ptr<ocpp::IdTagInfo> idTagInfo = OcppGetIdTagInfo(payloadObj, ocpp::JsonField::idTagInfo);
            int callMsgTransId = OcppGetTransactionId(callMsg->getPayload().c_str());
            if (callMsgTransId <= 0)
                throw std::runtime_error("Unable to parse transactionId field");
            if (idTagInfo) { // optional
                // IdTag can be found from StartTransction.req
                QString idTag = ocppGetIdTag(callMsg->getPayload().c_str());
                authCache.updateEntry(idTag, *idTagInfo);
                authCacheFlushTimer.start();
                emit stopTransactionResponse(idTagInfo->authStatus, QString::number(callMsgTransId));
                // TODO: handle case where authorization is not accepted.
            }
            else {
                emit stopTransactionResponse(ocpp::AuthorizationStatus::Accepted, QString::number(callMsgTransId));
            }

            ocppConnectionStatus = true;
            emit connStatusOnlineIndicator();
        }
        catch (std::runtime_error &rerr) {
            qCWarning(OcppComm) << "StopTransaction.conf: " << rerr.what();
        }
        break;
    case ocpp::CallActionType::DataTransferTimeValue:
    {
        QString dataTransferStatus = OcppGetItemString(payloadObj, ocpp::JsonField::status);
        if (dataTransferStatus == "Accepted") {
            emit OcppResponse("TimeValue Accepted");
        }
        else {
            qCWarning(OcppComm) << "DataTransfer.conf status is not good: " << dataTransferStatus;
            emit OcppResponse("TimeValue Rejected");
        }
        QString data = OcppGetItemString(payloadObj, ocpp::JsonField::data);
        if (!(data.isNull() || data.isEmpty())) // optional
            qCDebug(OcppComm) << "DataTransfer.conf.data: " << data;

        ocppConnectionStatus = true;
        emit connStatusOnlineIndicator();
    }
        break;
    case ocpp::CallActionType::FirmwareStatusNotification:
        // No fields defined
        qCDebug(OcppComm) << callMsg->getAction().data() << " OK";

        ocppConnectionStatus = true;
        emit connStatusOnlineIndicator();
        break;
    default:
        qCDebug(OcppComm) << callMsg->getAction().data()  << " Nothing to do";

        ocppConnectionStatus = true;
        emit connStatusOnlineIndicator();
        break;
    }
    //TODO:
}

void OcppClient::handleCallErrorMessage(std::shared_ptr<ocpp::CallErrorMessage> msg,
                                        QSharedPointer<ocpp::CallMessage> callMsg)
{
    std::istringstream iss(callMsg->getAction());
    ocpp::CallActionType callAction;
    iss >> callAction;
    if (callAction == ocpp::CallActionType::StopTransaction)
    { //不允许拒绝该指令:
        // TODO: How to force remove transaction?
    }

    qCWarning(OcppComm) << "Error Code: " << msg->getErrorCode().c_str();
    qCWarning(OcppComm) << "Error Description: " << msg->getErrorDescription().c_str();
    qCWarning(OcppComm) << "Error Details: " << msg->getPayload().c_str();
}

ocpp::LocalAuthorizationListVersion OcppClient::getLocalAuthListVer()
{
    ocpp::LocalAuthorizationListVersion listVer = ocpp::LocalAuthListNotAvail;
    try {
        LocalAuthList &authList = LocalAuthList::getInstance();
        listVer = authList.getVersion();
    }
    catch (std::runtime_error &re) {
        qCWarning(OcppComm) << "OcppClient::getLocalAuthListVer(): " << re.what();
        listVer = ocpp::LocalAuthListNotAvail;
    }

    return listVer;
}

// Convert Json object into AuthorizationData
std::vector<ocpp::AuthorizationData> OcppClient::jsonArrayToAuthData(const QJsonArray &jsonArr)
{
    std::vector<ocpp::AuthorizationData> newData;
    for (QJsonValue el : jsonArr) {
        if (el.isObject()) {
            QJsonObject aDataObj = el.toObject();
            QString idTagStrVal = OcppGetItemString(aDataObj, ocpp::JsonField::idTag);
            if (idTagStrVal.isNull() || idTagStrVal.isEmpty())
                throw std::runtime_error("AuthorizationData.idTag is empty or null");
            std::unique_ptr<ocpp::IdTagInfo> idTagInfo = OcppGetIdTagInfo(aDataObj, ocpp::JsonField::idTagInfo);
            if (!idTagInfo)
                throw std::runtime_error("AuthorizationData.idTagInfo is invalid or missing");
            ocpp::AuthorizationData aData(idTagStrVal.toLocal8Bit());
            aData.idTagInfo = std::move(idTagInfo);
            newData.push_back(std::move(aData));
        }
        else {
            throw std::runtime_error("AuthorizationData array element is not JsonObject type.");
        }
    }
    return newData;
}


//=============================================================================

void OcppClient::ocppSendBootNotification()
{
    QString cpVendor("?"), cPModel("?"), cpSerialNo("?");
    if(GetSetting(QStringLiteral("[CPVENDOR]"), cpVendor)
       &&GetSetting(QStringLiteral("[CPSERIALNO]"), cpSerialNo)
       &&GetSetting(QStringLiteral("[CPMODEL]"), cPModel))
    {
        ;
    }
    QString fwVer = charger::getSoftwareVersion();
    QString uniqueId = newOcppMessageId();
    ocpp::CallActionType callAction = ocpp::CallActionType::BootNotification;
    QString payloadStr = QString(_BootNotification).arg(cpVendor).arg(cPModel)
            .arg(cpSerialNo).arg(fwVer);
    QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
    sendTextMessage(callMsg, true);
}

void OcppClient::ocppSendHeartbeat()
{
    if(!processHeartbeatWithResponse())
        return;
    QString uniqueId = newOcppMessageId();
    ocpp::CallActionType callAction = ocpp::CallActionType::Heartbeat;
    QString payloadStr ="";
    QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
    if(!connectionFailTimer.isActive() && !isOffline())
    {
        connectionFailTimer.setSingleShot(true);
        connectionFailTimer.start(std::chrono::duration_cast<std::chrono::milliseconds>(hbTimerInterval * 3));
    }
    sendTextMessage(callMsg, true);
}

bool OcppClient::processHeartbeatWithResponse()
{
    if(heartbeatTriggerCount < defaultHeartbeatTriggerCount)
         return false;
    isSendHeaetbeatState = true;
    return  true;
}

void OcppClient::ocppParkingDataTransfer(QString pState, float dist)
{
    QString uniqueId = newOcppMessageId();
    int uIdLength = uniqueId.length();
    uniqueId = uniqueId.replace(uIdLength-3,2,"99");

    //车位是否被占用
    //QString payloadStr = QString("\"parking\":\"1%\",").arg(
    //            (DistanceSensorThread::GetDistance()*100 > charger::GetDistThreshold()
    //            ?"Free":"Ocuppied"));

    //qCWarning(OcppComm) << "OcppClient::distance: " << DistanceSensorThread::GetDistance()*100 - charger::GetDistThreshold();
    ocpp::CallActionType callAction =ocpp::CallActionType::DataTransferTimeValue;
    QString payloadStr = QString(_DataTransfer_w_Parking)
            .arg(m_ChargerA.IdTag)
            .arg(pState)
            .arg(dist);

    QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
    sendTextMessage(callMsg);
}

void OcppClient::ocppSendStatusNotification()
{
    if (chargerRegStatus == ocpp::RegistrationStatus::Accepted) {
        QString uniqueId = newOcppMessageId();
        ocpp::CallActionType callAction = ocpp::CallActionType::StatusNotification;
        QString cpErrorCodeStr = ocpp::chargePointErrorCodeStrs.at(m_ChargerA.GetErrorCode()).data();
        QString payloadStr = QString(_StatusNotification).arg("1").arg(cpErrorCodeStr)
                .arg(ocpp::ChargerStatusStrs.at(m_ChargerA.Status).data()).arg(m_ChargerA.VendorErrorCode);
        QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
        sendTextMessage(callMsg, true);
    }
    else {
        qCWarning(OcppComm) << "Not queuing ocpp::StatusNotification before BootNotification is accepted";
    }
}

void OcppClient::ocppSendStartTransaction(const QDateTime startDt, const unsigned long startMeterVal, const QString tagId)
{
    QString uniqueId = newOcppMessageId();
    ocpp::CallActionType callAction = ocpp::CallActionType::StartTransaction;
    int meterStartVal = charger::forceOcppMeterValueKWh()?
                (startMeterVal / 10)
              : CCharger::meterValToWh(startMeterVal);
    QString payloadStr;
    if(!m_ChargerA.IsBookingState())
    {
        payloadStr = QString(_StartTransaction).arg("1").arg(tagId)
            .arg(meterStartVal).arg(startDt.toString(Qt::DateFormat::ISODate));
    }
    else
    {
        payloadStr = QString(_StartTransactionwithReserve).arg("1").arg(tagId)
            .arg(meterStartVal).arg(startDt.toString(Qt::DateFormat::ISODate)).arg(m_ChargerA.ReservationId);
        m_ChargerA.CancelBooking();
    }
    QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
    sendTextMessage(callMsg);
}

void OcppClient::ocppSendStopTransaction(const QString transId, const QDateTime stopDt, const unsigned long stopMeterVal, const ocpp::Reason reason, const QString tagId)
{
    QString stopDtStr = stopDt.toString(Qt::DateFormat::ISODateWithMs);
    QString uniqueId = newOcppMessageId();
    ocpp::CallActionType callAction = ocpp::CallActionType::StopTransaction;
    int meterStopVal = charger::forceOcppMeterValueKWh()?
                (stopMeterVal / 10)
              : CCharger::meterValToWh(stopMeterVal);
    QString payloadStr = QString(_StopTransaction).arg(tagId).arg(meterStopVal)
            .arg(stopDtStr).arg(transId).arg(ocpp::reasonStrs.at(reason).data());
    QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
    sendTextMessage(callMsg);
}

void OcppClient::ocppSendMeterValues(const QString transId, const EvChargingDetailMeterVal &meterVal)
{
    QString uniqueId = newOcppMessageId();
    ocpp::CallActionType callAction = ocpp::CallActionType::MeterValues;
    QDateTime utcNow = meterVal.meteredDt;
    double qVal = charger::forceOcppMeterValueKWh()?
                (m_ChargerA.ChargedQ / 10.0)
              : CCharger::meterValToWh(m_ChargerA.ChargedQ);
    QString unit = charger::forceOcppMeterValueKWh()? "kWh" : "Wh";
    //txt =QString(_MeterValues).arg(uniqueId).arg("1").arg(m_ChargerA.TransactionId).arg(dt.toString("yyyy-MM-ddTHH:mm:ss.zzzZ")).arg(m_ChargerA.ChargedQ/1.0, 0, 'f', 1);
    //txt =QString(_MeterValues).arg(uniqueId).arg("1").arg(m_ChargerA.TransactionId).arg(dt.toString("yyyy-MM-ddTHH:mm:ss.zzzZ")).arg(m_ChargerA.ChargedQ/1.0, 0, 'f', 1).arg(
    //            (m_ChargerA.ChargerNewRaw.Ia+m_ChargerA.ChargerNewRaw.Ib+m_ChargerA.ChargerNewRaw.Ic)/1000.0, 0,'f',3);
    QString payloadStr;
    if(isSuspended > 0){
        payloadStr = QString(_MeterValues_w_Intr).arg("1").arg(transId).arg(utcNow.toString(Qt::DateFormat::ISODateWithMs))
                .arg(qVal, 0, 'f', 1).arg(unit).arg(meterVal.P/100.0, 0, 'f', 2)
                .arg(meterVal.I[0]/1000.0, 0,'f',3).arg(meterVal.I[1]/1000.0, 0,'f',3).arg(meterVal.I[2]/1000.0, 0,'f',3)
                .arg(meterVal.V[0]/100.0, 0,'f',2).arg(meterVal.V[1]/100.0, 0,'f',2).arg(meterVal.V[2]/100.0, 0,'f',2)
                .arg(meterVal.Pf/1000.0, 0,'f',3).arg(meterVal.Freq/100.0, 0, 'f', 2);
    }else{
        payloadStr = QString(_MeterValues).arg("1").arg(transId).arg(utcNow.toString(Qt::DateFormat::ISODateWithMs))
                .arg(qVal, 0, 'f', 1).arg(unit).arg(meterVal.P/100.0, 0, 'f', 2)
                .arg(meterVal.I[0]/1000.0, 0,'f',3).arg(meterVal.I[1]/1000.0, 0,'f',3).arg(meterVal.I[2]/1000.0, 0,'f',3)
                .arg(meterVal.V[0]/100.0, 0,'f',2).arg(meterVal.V[1]/100.0, 0,'f',2).arg(meterVal.V[2]/100.0, 0,'f',2)
                .arg(meterVal.Pf/1000.0, 0,'f',3).arg(meterVal.Freq/100.0, 0, 'f', 2);
    }
    payloadStr.remove(' ');
    QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
    sendTextMessage(callMsg, true);
}

void OcppClient::raiseOcppAuthOfflineSignal()
{
    emit OcppAuthorizeReqCannotSend();
}

void OcppClient::ocppSendAuthorize(const QString tagId)
{
    if (m_OcppClient.isOffline()) {
        QTimer::singleShot(0, this, &OcppClient::raiseOcppAuthOfflineSignal);
    }
    else {
        QString uniqueId = newOcppMessageId();
        ocpp::CallActionType callAction = ocpp::CallActionType::Authorize;
        QString payloadStr = QString(_Authorize).arg(tagId);
        QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
        sendTextMessage(callMsg);
    }
}

void OcppClient::ocppSendDataTransfer()
{
    QString uniqueId = newOcppMessageId();
    ocpp::CallActionType callAction =ocpp::CallActionType::DataTransferTimeValue;
    QString payloadStr = QString(_DataTransfer).arg((int)(m_ChargerA.ChargeTimeValue/60)).arg(m_ChargerA.IdTag);
    QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
    sendTextMessage(callMsg);
}

void OcppClient::ocppSendFirmwareStatusNotification(ocpp::FirmwareStatus status)
{
    QString uniqueId = newOcppMessageId();
    ocpp::CallActionType callAction = ocpp::CallActionType::FirmwareStatusNotification;
    std::ostringstream oss;
    oss << status;
    QString payloadStr = QString(_SimplyStatusJson).arg(oss.str().data());
    QSharedPointer<ocpp::CallMessage> callMsg = QSharedPointer<ocpp::CallMessage>::create(uniqueId, callAction, payloadStr);
    sendTextMessage(callMsg);
}

void OcppClient::ocppReplyUnlockConnector(const QString &uniqueId, const ocpp::UnlockStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::unlockStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyGetLocalListVersion(const QString &uniqueId, const int listVersion)
{
    QString payloadStr = QString(_R_GetLocalListVersion).arg(listVersion);
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplySendLocalList(const QString &uniqueId, const ocpp::UpdateStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::updateStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyRemoteStartTransaction(const QString &uniqueId, const ocpp::RemoteStartStopStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::remoteStartStopStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyRemoteStopTransaction(const QString &uniqueId, const ocpp::RemoteStartStopStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::remoteStartStopStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyClearCache(const QString &uniqueId, const ocpp::ClearCacheStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::clearCacheStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyChangeAvailability(const QString &uniqueId, const ocpp::AvailabilityStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::availabilityStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyChangeConfiguration(const QString &uniqueId, const ocpp::ConfigurationStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::configurationStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyReset(const QString &uniqueId, const ocpp::ResetStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::resetStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyGetConfiguration(const QString &uniqueId,
                const std::vector<ocpp::KeyValue> &cfgKeys, const std::vector<QString> &unknownKeys)
{
    // Payload JSON has 2 array fields, one correspond to the known configuration keys, the other is unknown/missing;
    QJsonObject payload;
    if (!cfgKeys.empty()) {
        // Make a QVariantList of Json-Objects
        // Json-Objects corresponds to the KeyValue(s)
        // JsonObject << QVariantMap << KeyValue
#if false
        QVariantList cfgList(cfgKeys.cbegin(), cfgKeys.cend());
#else
        QVariantList cfgList;
        for (auto i = cfgKeys.cbegin(); i != cfgKeys.cend(); i++)
            cfgList.push_back(QVariant(ocpp::keyValueToJson(*i)));
#endif
        QJsonArray cfgArr = QJsonArray::fromVariantList(cfgList);
        payload.insert(ocpp::jsonFieldStr.at(ocpp::JsonField::configurationKey).data(), cfgArr);
    }
    if (!unknownKeys.empty()) {
#if false // only available Qt 5.14 +
        QStringList keyList(unknownKeys.cbegin(), unknownKeys.cend());
#else
        QStringList keyList;
        for (auto i = unknownKeys.cbegin(); i != unknownKeys.cend(); i++)
            keyList.push_back(*i);
#endif
        QJsonArray keyArr = QJsonArray::fromStringList(keyList);
        payload.insert(ocpp::jsonFieldStr.at(ocpp::JsonField::unknownKey).data(), keyArr);
    }

    QJsonDocument jsonDoc(payload);
    QByteArray payloadBytes = jsonDoc.toJson(QJsonDocument::JsonFormat::Compact);
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadBytes);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyReserveNow(const QString &uniqueId, const ocpp::ReservationStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::reservationStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyCancelReservation(const QString &uniqueId, const ocpp::CancelReservationStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::cancelReservationStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplySetChargingStatus(const QString &uniqueId, const ocpp::ChargingProfileStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::chargingProfileStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyUpdateFirmware(const QString &uniqueId, const ocpp::NotdefinedStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::notdefinedStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyTriggerMessage(const QString &uniqueId, const ocpp::TriggerMessageStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::TriggerMessageStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::ocppReplyDataTransfer(const QString &uniqueId, const ocpp::DataTransferStatus status)
{
    QString payloadStr = QString(_SimplyStatusJson).arg(ocpp::DataTransferStatusStrs.at(status).data());
    QSharedPointer<ocpp::CallResultMessage> callResMsg = QSharedPointer<ocpp::CallResultMessage>::create(uniqueId, payloadStr);
    sendTextMessage(callResMsg);
}

void OcppClient::addStatusNotificationReq()
{
    ocppSendStatusNotification();
}

void OcppClient::addFirmwareStatusNotificationReq(ocpp::FirmwareStatus status)
{
    ocppSendFirmwareStatusNotification(status);
}

void OcppClient::addAuthorizeReq(const QString tagId, const bool checkCache)
{
    // Check Authorization Cache first
    if (checkCache && authCache.checkValid(tagId)) {
        QTimer::singleShot(0, this, &OcppClient::onAuthCacheLookupSucc);
    }
    else {
        ocppSendAuthorize(tagId);
    }
}

void OcppClient::addDataTransferReq()
{
    ocppSendDataTransfer();
}

void OcppClient::addStartTransactionReq(const QDateTime startDt, const unsigned long startMeterVal,
                               const QString tagId)
{
    ocppSendStartTransaction(startDt, startMeterVal, tagId);
}

void OcppClient::addStopTransactionReq(const QString transId, const QDateTime stopDt, const unsigned long stopMeterVal,
                              const ocpp::Reason reason, const QString tagId)
{
    ocppSendStopTransaction(transId, stopDt, stopMeterVal, reason, tagId);
}

void OcppClient::addMeterValuesReq(const QString transId, const EvChargingDetailMeterVal &meterVal)
{
    ocppSendMeterValues(transId, meterVal);
}

void OcppClient::addToPendingInCallRequests(QSharedPointer<ocpp::CallMessage> callMsg)
{
    pendingInCallReqs.append(callMsg);
}

QSharedPointer<ocpp::CallMessage> OcppClient::removeFirstPendingInCallRequestsOf(const ocpp::CallActionType action)
{
    QSharedPointer<ocpp::CallMessage> callMsg;
    for(auto itr = pendingInCallReqs.begin(); itr != pendingInCallReqs.end(); itr++) {
        if (itr->get()->getAction().compare(ocpp::callActionStrs.at(action).data()) == 0) {
            callMsg = *itr;
            pendingInCallReqs.erase(itr);
            break;
        }
    }
    return callMsg;
}

bool OcppClient::hasPendingInCallRequestsOf(const ocpp::CallActionType action)
{
    bool found = false;
    for(auto itr = pendingInCallReqs.begin(); itr != pendingInCallReqs.end(); itr++) {
        if (itr->get()->getAction().compare(ocpp::callActionStrs.at(action).data()) == 0) {
            found = true;
            break;
        }
    }
    return found;
}

//=============================================================================

QString OcppClient::GetLocalIP()
{
    if(IP_Address =="")
    {
        IP_Address =ws.localAddress().toString();
    }
    //qCDebug(OcppComm) <<"IP_Address:" <<IP_Address;
    return IP_Address;
}

QString OcppClient::newOcppMessageId(void)
{
    QDateTime curdt =QDateTime::currentDateTime();
    return curdt.toString("yyMMddHHmmsszzz");
}


RmtStartTxExec::RmtStartTxExec()
{
    clear();
}

void RmtStartTxExec::clear()
{
    pending = false;
    idTag.clear();
    chargeTimeVal = 0; // unset
    currentLimit = -1.0; // unset
}

void RmtStartTxExec::schedule(const QString tagId, const unsigned long chargeTime, const double maxCurrent)
{
    idTag = tagId;
    if (chargeTime > 0)
        chargeTimeVal = chargeTime;
    if (maxCurrent >= 0.0)
        currentLimit = maxCurrent;
    pending = true;
}

void RmtStartTxExec::go()
{
    if (m_ChargerA.IsIdleState() || m_ChargerA.IsBookingState()) {
        m_ChargerA.IdTag = idTag;
        // Initialize with default values, override with OCPP Call argument values
        m_ChargerA.ChargeTimeValue = 60;
        if (chargeTimeVal > 0)
            m_ChargerA.ChargeTimeValue = chargeTimeVal;
        if (currentLimit >= 0.0)
            m_ChargerA.SetProfileCurrent(static_cast<int>(currentLimit));
        qCDebug(OcppComm) <<"***RemoteStartTransaction:***"
                << " TagId: " << m_ChargerA.IdTag
                << " Time: " << m_ChargerA.ChargeTimeValue
                << " Current-Limit: " << m_ChargerA.GetCurrentLimit();
        m_ChargerA.RemainTime = m_ChargerA.ChargeTimeValue;
        m_ChargerA.StartDt = QDateTime(); // clear value
        if(m_ChargerA.ChargeTimeValue > 0)
            m_ChargerA.StartCharge();

        clear();
    }
    else {
        qCWarning(OcppComm) << "RmtStartTxExec::go() Charger State is not yet Idle. Try again";
        QTimer::singleShot(1000, this, &RmtStartTxExec::go);
    }

}

void OcppClient::onConnectionFailTimeout()
{
    emit connStatusOfflineIndicator();
    qWarning() << "Hearbeat timeout, offline now";
    ocppConnectionStatus = false;
    st = State::Idle;
    emit disconnected();
}

void OcppClient::incrHeartbeatTriggerCount()
{
    if(heartbeatTriggerCount < defaultHeartbeatTriggerCount)
         heartbeatTriggerCount++;
}

void  OcppClient::addParkingDataTransferReq(QString pState, float dist)
{
    ocppParkingDataTransfer(pState, dist);
}
