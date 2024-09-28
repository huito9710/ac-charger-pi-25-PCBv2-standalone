#ifndef OCPPCLIENT_H
#define OCPPCLIENT_H

#include <QtCore/QObject>
#include <QWebSocket>
#include <QtNetwork/QSslError>
#include <QAbstractSocket>
#include <QSslPreSharedKeyAuthenticator>
#include <QAuthenticator>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include "OCPP/ocppTypes.h"
#include "OCPP/rpcmessage.h"
#include "OCPP/ocppoutmessagequeue.h"
#include "authorizationcache.h"
#include "evchargingtransaction.h"

// Execute the Remote-Start-Transaction Request
class RmtStartTxExec : public QObject {
    Q_OBJECT
public:
    RmtStartTxExec();
    void schedule(const QString tagId, const unsigned long chargeTime, const double maxCurrent);
    bool isScheduled() { return pending; };
public slots:
    void go();
private:
    QString idTag;
    unsigned long chargeTimeVal;
    double currentLimit;
    bool pending;
    void clear();
};

class OcppClient : public QObject
{
    Q_OBJECT
public:
    static OcppClient &instance() { static OcppClient ocppCli; return ocppCli; }
    ~OcppClient();
    void ConnectServer();
    void DisconnectServer();
    void CloseWebSocket();

signals:
    void    connected();
    void    connectFailed();
    void    disconnected();
    void    callTimedout();
    void    bootNotificationAccepted(); // BootNotification accepted
    void    OcppAuthorizeResponse(bool statusAvail, ocpp::AuthorizationStatus authStatus);
    void    OcppAuthorizeReqCannotSend(); // currently offline
    void    OcppResponse(QString result);
    void    scheduleConnUnavailChange();
    void    cancelScheduleConnUnavailChange();
    void    startTransactionResponse(ocpp::AuthorizationStatus status, const QString idTag, const QString transId = QString());
    void    stopTransactionResponse(ocpp::AuthorizationStatus status, const QString transId);
    void    meterValueResponse(const QString transId);
    void    meterSampleIntervalCfgChanged();
    void    serverDateTimeReceived(const QDateTime dt);
    void    checkunlock();
    void    hardreset();
    void    connStatusOnlineIndicator();
    void    connStatusOfflineIndicator();
    void    popUpMsgReceived();
    void    cancelPopUpMsgReceived();
    void    changeLedLight(int color);

public:
    std::chrono::seconds    getMeterValueSampleInterval() { return meterIntervalKeyValue; };

    uint    m_CableTimeoutKeyValue;

    bool    isConnectedOrConnecting() { return (ws.isValid() || st == State::Connecting); } // to OCPP CS
    bool    isOffline() { return (ocppConnectionStatus == false); }
    bool    isChargerAccepted() { return chargerRegStatus == ocpp::RegistrationStatus::Accepted; }

    void    OcppManage();

    void    addStatusNotificationReq();
    void    addFirmwareStatusNotificationReq(ocpp::FirmwareStatus status);
    void    addAuthorizeReq(const QString tagId, const bool checkCache = true);
    void    addDataTransferReq();
    void    addStartTransactionReq(const QDateTime startDt, const unsigned long startMeterVal,
                                   const QString tagId = QString());
    void    addStopTransactionReq(const QString transId, const QDateTime stopDt, const unsigned long stopMeterVal,
                                  const ocpp::Reason reason, const QString tagId = QString());
    void    addMeterValuesReq(const QString transId, const EvChargingDetailMeterVal &meterVal);

    void    addParkingDataTransferReq(QString pState, float dist);

    QString GetLocalIP();

private slots:
    void onConnected();
    void onDisconnected();
    void onStateChanged(QAbstractSocket::SocketState sockState);
    void onPreSharedKeyAuthRequired(QSslPreSharedKeyAuthenticator *authenticator);
    void onProxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
    void onHeartBeatTimeout();
    void onTextMessageReceived(QString message);
    void resetHeartbeatTriggerCount();
    void onSslErrors(const QList<QSslError> &errors);
    void onWsTimeout();
    void onBootNotifTimeout();
    void onAuthCacheFlushTimeout();
    void onAuthCacheLookupSucc();
    void onConnAvailChanged(bool newVal);
    void onMsgSent(QSharedPointer<ocpp::RpcMessage> sentMsg);
    void onMsgError(OcppOutMessageQueue::MessageSendError error, QSharedPointer<ocpp::RpcMessage> outMsg);
    void onMsgNoReplyError(QSharedPointer<ocpp::RpcMessage> outMsg);
    void raiseOcppAuthOfflineSignal();
    void onConnectionFailTimeout();
    void incrHeartbeatTriggerCount();
    bool processHeartbeatWithResponse();
    void stopHeartbeatTimer();

private:
    enum class State : int {
        Idle,
        Connecting,
        Connected,
        Disconnecting,
        CallMessageSent,
        CallResponseRcvd,
    };

    State      st;
    ocpp::RegistrationStatus chargerRegStatus;
    QWebSocket ws;
    OcppOutMessageQueue ocppMsgOutQ;
    QString    IP_Address ="";      //本地IP
    AuthorizationCache &authCache;
    QTimer     connTimer;
    QTimer     bootNotifTimer;      // retry bootNotification
    const std::chrono::milliseconds defaultHeartBeatInterval;
    std::chrono::milliseconds hbTimerInterval;
    QTimer     heartBeatTimer;
    QTimer     heartbeatTriggerCountTimer;
    u8         heartbeatTriggerCount = 0;
    bool       isSendHeaetbeatState = false;
    QTimer     authCacheFlushTimer;
    bool       connUnavailPending;
    const std::chrono::seconds connMaxWait;
    const std::chrono::seconds defaultBootNotifRetryInterval;
    const std::chrono::minutes authCacheFlushInterval;
    // Call requests being processed outside the incoming message handler.
    QList<QSharedPointer<ocpp::CallMessage>> pendingInCallReqs;
    const int defaultMeterValueSampleIntervalValue = 10; // 10 seconds
    std::chrono::seconds meterIntervalKeyValue;    //0 is no transmit
    RmtStartTxExec remoteStartTxAction;
    const u8 defaultHeartbeatTriggerCount;

    explicit OcppClient(QObject *parent = Q_NULLPTR);
    void OcppDataReceived(QString& message);
    ocpp::LocalAuthorizationListVersion getLocalAuthListVer();
    std::vector<ocpp::AuthorizationData> jsonArrayToAuthData(const QJsonArray &jsonArr);
    // Returns QString() - Null if field not present, value not string, value is null
    // Returns QString("") or QString("abc"): according to the actual value
    QString OcppGetItemString(const QJsonObject &json, const ocpp::JsonField key);
    // Returns fallBackVal if field not found or not a integer
    int OcppGetItemInt(const QJsonObject &json, const ocpp::JsonField key, int fallbackVal = 0);
    // Returns fallbackVal if field not found or not a double
    double OcppGetItemDouble(const QJsonObject &json, const ocpp::JsonField key, double fallbackVal = 0.0);
    QJsonObject OcppGetItemObj(const QJsonObject &json, const ocpp::JsonField key);
    // Return a QDateTime with null-value inside if field not present, or not ISO 8601
    QDateTime OcppGetItemDateTime(const QJsonObject &json, const ocpp::JsonField key);
    QJsonArray OcppGetItemArray(const QJsonObject &json, const ocpp::JsonField key);
    std::unique_ptr<ocpp::IdTagInfo> OcppGetIdTagInfo(const QJsonObject &json, const ocpp::JsonField key);
    long OcppGetTransactionId(const QByteArray &jsonStr);
    QString ocppGetIdTag(const QByteArray &jsonStr);
    void ocppSendBootNotification();
    void ocppSendHeartbeat();
    void ocppParkingDataTransfer(QString pState, float dist);
    void ocppSendStatusNotification();
    void ocppSendStartTransaction(const QDateTime startDt, const unsigned long startMeterVal, const QString tagId);
    void ocppSendStopTransaction(const QString transId, const QDateTime stopDt, const unsigned long stopMeterVal, const ocpp::Reason reason, const QString tagId);
    void ocppSendMeterValues(const QString transId, const EvChargingDetailMeterVal &meterVal);
    void ocppSendAuthorize(const QString tagId);
    void ocppSendDataTransfer();
    void ocppSendFirmwareStatusNotification(ocpp::FirmwareStatus status);
    void ocppReplyUnlockConnector(const QString &uniqueId, const ocpp::UnlockStatus status);
    void ocppReplyGetLocalListVersion(const QString &uniqueId, const int listVersion);
    void ocppReplySendLocalList(const QString &uniqueId, const ocpp::UpdateStatus status);
    void ocppReplyRemoteStartTransaction(const QString &uniqueId, const ocpp::RemoteStartStopStatus status);
    void ocppReplyRemoteStopTransaction(const QString &uniqueId, const ocpp::RemoteStartStopStatus status);
    void ocppReplyClearCache(const QString &uniqueId, const ocpp::ClearCacheStatus status);
    void ocppReplyChangeAvailability(const QString &uniqueId, const ocpp::AvailabilityStatus status);
    void ocppReplyChangeConfiguration(const QString &uniqueId, const ocpp::ConfigurationStatus status);
    void ocppReplyReset(const QString &uniqueId, const ocpp::ResetStatus status);
    void ocppReplyGetConfiguration(const QString &uniqueId,
                                   const std::vector<ocpp::KeyValue> &cfgKeys = {},
                                   const std::vector<QString> &unknownKeys = {});
    void ocppReplyReserveNow(const QString &uniqueId, const ocpp::ReservationStatus status);
    void ocppReplyCancelReservation(const QString &uniqueId, const ocpp::CancelReservationStatus status);
    void ocppReplySetChargingStatus(const QString &uniqueId, const ocpp::ChargingProfileStatus status);
    void ocppReplyUpdateFirmware(const QString &uniqueId, const ocpp::NotdefinedStatus status);
    void ocppReplyTriggerMessage(const QString &uniqueId, const ocpp::TriggerMessageStatus status);
    void ocppReplyDataTransfer(const QString &uniqueId, const ocpp::DataTransferStatus status);
    QString newOcppMessageId(void);
    void sendTextMessage(QSharedPointer<ocpp::RpcMessage> rpcMsg, bool replace = false);
    void handleCallMessage(std::shared_ptr<ocpp::CallMessage> msg);
    void handleCallResultMessage(std::shared_ptr<ocpp::CallResultMessage> msg,
                                 QSharedPointer<ocpp::CallMessage> callMsg);
    void handleCallErrorMessage(std::shared_ptr<ocpp::CallErrorMessage> msg,
                                QSharedPointer<ocpp::CallMessage> callMsg);
    void addToPendingInCallRequests(QSharedPointer<ocpp::CallMessage> callMsg);
    QSharedPointer<ocpp::CallMessage> removeFirstPendingInCallRequestsOf(const ocpp::CallActionType action);
    bool hasPendingInCallRequestsOf(const ocpp::CallActionType action);

    bool ocppConnectionStatus = false;
    QTimer     connectionFailTimer;
};

#endif // OCPPCLIENT_H
