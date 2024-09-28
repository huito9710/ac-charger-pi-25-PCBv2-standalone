#ifndef OCPPOUTMESSAGEQUEUE_H
#define OCPPOUTMESSAGEQUEUE_H

#include <chrono>
#include <QObject>
#include <QWebSocket>
#include <QList>
#include <QTimer>
#include "OCPP/rpcmessage.h"
#include "OCPP/callmessage.h"
#include "OCPP/callresultmessage.h"
#include "OCPP/callerrormessage.h"

///
/// \brief The OcppOutMessageQueue class
///
/// \abstract Non-Transactional messages will be delivered first, to make sure
/// 'time-critical' requests like Authorize.req can be made ASAP.
///
class OcppOutMessageQueue : public QObject
{
    Q_OBJECT
public:
    enum class MessageSendError: int {
        ServerOffline,
        SendTimedOut,
        NoReplyTimedOut, // No-reply to call message within a period of time
    };
    enum class SendState: int {
        Idle,
        Scheduled,
        Sent,
        Retrying,
        WaitReply
    };
public:
    explicit OcppOutMessageQueue(QWebSocket &ws, QObject *parent = nullptr);
    void initParams(int numRetries, std::chrono::seconds retryInterval,
              std::chrono::seconds noReplyTimeout);
    void add(QSharedPointer<ocpp::RpcMessage> outMsg);
    void addReplaceExisting(QSharedPointer<ocpp::CallMessage> outMsg);
    QSharedPointer<ocpp::RpcMessage> removeSentCallMessage(const QString &id); // return true if corresponding message exist and removed
signals:
    void msgSent(QSharedPointer<ocpp::RpcMessage> sentMsg);
    void msgError(MessageSendError error, QSharedPointer<ocpp::RpcMessage> outMsg);
    void msgNoReplyError(QSharedPointer<ocpp::RpcMessage> outMsg);

private:
    enum class MsgQueueType: int {
        NonTransactional,
        Transactional
    };
private:
    QWebSocket &ws;
    SendState st;
    bool csOnline;
    MsgQueueType currentQ;
    // non-transactional messages
    QList<QSharedPointer<ocpp::RpcMessage>> outNonTransMsgQ;
    QList<QSharedPointer<ocpp::RpcMessage>> sentNonTransMsgQ;
    // transactional messages (Start, Stop, MeterValues)
    QList<QSharedPointer<ocpp::RpcMessage>> outTransMsgQ;
    QList<QSharedPointer<ocpp::RpcMessage>> sentTransMsgQ;
    QTimer sendMsgTimer;
    int retryCnt;
    int maxRetries;
    int maxWaitReplyRetries;
    QMap<std::string, int> waitReplyRecordMap;
    std::chrono::seconds sendTimeoutPeriod;
    std::chrono::seconds sendRetryItvl;
    std::chrono::milliseconds noReplyTimeoutPeriod;

private:
    void stopSendMsgTimer() { if (sendMsgTimer.isActive()) sendMsgTimer.stop(); };
    void startMsgSendToSocketTimer();
    void startMsgSendRetryTimer();
    void startNoReplyTimer();
    void sendMessage();
    void scheduleNextIfQNotEmpty();
    void processNextMessage();
    void restoreSentMsgToOutQ();
    void removeRealTimeMsgFromQ(QList<QSharedPointer<ocpp::RpcMessage>> &msgQ);

private slots:
    void onSendMessageTimeout();
    void onBytesWritten(qint64 numBytes);
    void onWsStateChanged(QAbstractSocket::SocketState sockState);
    bool isTransactionalMsg(QSharedPointer<ocpp::RpcMessage>);
};

#endif // OCPPOUTMESSAGEQUEUE_H

