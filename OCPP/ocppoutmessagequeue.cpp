#include "ocppoutmessagequeue.h"
#include <QtGlobal>
#include <QDebug>
#include <QWebSocket>
#include <QAbstractSocket>
#include "logconfig.h"
#include "gvar.h"

static constexpr int defaultNumSendRetries = 3;
static constexpr int defaultNumWaitReplyRetries = 3;
static constexpr std::chrono::seconds defaultSendTimeoutPeriodSecs = std::chrono::seconds(3);
static constexpr std::chrono::seconds defaultSendRetryPeriodSecs = std::chrono::seconds(3);
static constexpr std::chrono::milliseconds defaultNoReplyTimeoutPeriodSecs = std::chrono::milliseconds(1500);

OcppOutMessageQueue::OcppOutMessageQueue(QWebSocket &ws, QObject *parent)
    : QObject(parent),
      ws(ws),
      st(SendState::Idle),
      csOnline(false),
      currentQ(MsgQueueType::NonTransactional),
      retryCnt(0),
      maxRetries(defaultNumSendRetries),
      maxWaitReplyRetries(defaultNumWaitReplyRetries),
      sendTimeoutPeriod(defaultSendTimeoutPeriodSecs),
      sendRetryItvl(defaultSendRetryPeriodSecs),
      noReplyTimeoutPeriod(defaultNoReplyTimeoutPeriodSecs)
{

    sendMsgTimer.setSingleShot(true);
    // period depends on usage
    connect(&sendMsgTimer, &QTimer::timeout, this, &OcppOutMessageQueue::onSendMessageTimeout);
    connect(&ws, &QWebSocket::bytesWritten, this, &OcppOutMessageQueue::onBytesWritten);
    // QWebSocket::error is a overloaded function, refer to Qt manual of how this works
    QObject::connect(&ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
                     [=](QAbstractSocket::SocketError error) {
                        qCWarning(OcppComm) << "OcppOutMessageQueue (State=" << static_cast<int>(st)
                            << "): WebSocket Socket-Error. " << error << ". " << this->ws.errorString();
                            // Do nothing for now and let the timer-handler do the job?
                        });
    QObject::connect(&ws, &QWebSocket::stateChanged, this, &OcppOutMessageQueue::onWsStateChanged);
    if (ws.state() == QAbstractSocket::SocketState::ConnectedState)
        csOnline = true;
}

void OcppOutMessageQueue::initParams(int numRetries, std::chrono::seconds retryInterval,
          std::chrono::seconds noReplyTimeout)
{
    maxRetries = numRetries;
    sendRetryItvl = retryInterval;
    noReplyTimeoutPeriod = noReplyTimeout;
}

void OcppOutMessageQueue::startMsgSendToSocketTimer()
{
    if (sendMsgTimer.isActive())
        sendMsgTimer.stop();
    sendMsgTimer.setInterval(sendTimeoutPeriod);
    sendMsgTimer.start();
}


void OcppOutMessageQueue::startMsgSendRetryTimer()
{
    if (sendMsgTimer.isActive())
        sendMsgTimer.stop();
    sendMsgTimer.setInterval(sendRetryItvl);
    sendMsgTimer.start();
}

void OcppOutMessageQueue::startNoReplyTimer()
{
    if (sendMsgTimer.isActive())
        sendMsgTimer.stop();
    sendMsgTimer.setInterval(noReplyTimeoutPeriod);
    sendMsgTimer.start();
}

bool OcppOutMessageQueue::isTransactionalMsg(QSharedPointer<ocpp::RpcMessage> msg)
{
    bool isTransactional = false;
    switch (msg->getType()) {
    case ocpp::RpcMessageType::Call:
    {
        QSharedPointer<ocpp::CallMessage> callMsg = msg.dynamicCast<ocpp::CallMessage>();
        std::istringstream iss(callMsg->getAction());
        ocpp::CallActionType callAction;
        iss >> callAction;
        switch (callAction) {
        case ocpp::CallActionType::StartTransaction:
        case ocpp::CallActionType::StopTransaction:
        case ocpp::CallActionType::MeterValues:
            isTransactional = true;
            break;
        default:
            isTransactional = false;
            break;
        }
    }
        break;
    default:
        break;
    }

    return isTransactional;
}

void OcppOutMessageQueue::add(QSharedPointer<ocpp::RpcMessage> outMsg)
{
    // only add transaction messages to queue when cs is offline
    if (csOnline || isTransactionalMsg(outMsg)) {
        QList<QSharedPointer<ocpp::RpcMessage>> &outMsgQ = isTransactionalMsg(outMsg)? outTransMsgQ : outNonTransMsgQ;
        bool listWasEmpty = outMsgQ.empty();

        outMsgQ.append(outMsg);
        if (listWasEmpty) {
            qCDebug(OcppComm) << "OcppOutMessageQueue::add " << outMsg->getId().c_str();
            scheduleNextIfQNotEmpty();
        }
    }
}

void OcppOutMessageQueue::addReplaceExisting(QSharedPointer<ocpp::CallMessage> outMsg)
{
    // only add transaction messages to queue when cs is offline
    if (csOnline || isTransactionalMsg(outMsg)) {
        // Remove existing(s) Call-Message of the same Action.
        // Special-Case: found at beginning of list, make sure the current
        // state is Idle (not sending / waiting for reply)
        QList<QSharedPointer<ocpp::RpcMessage>> &outMsgQ = isTransactionalMsg(outMsg)? outTransMsgQ : outNonTransMsgQ;
        for (auto itr = outMsgQ.begin(); itr != outMsgQ.end(); itr++) {
            if (itr->get()->getType() == ocpp::RpcMessageType::Call) {
                QSharedPointer<ocpp::CallMessage> msg = itr->dynamicCast<ocpp::CallMessage>();
                if (msg->getAction() == outMsg->getAction()) {
                    if (itr == outMsgQ.begin() && (st != SendState::Idle && st != SendState::Scheduled)) {
                        qCWarning(OcppComm) << "Not replacing " << msg->getId().data() << " with " << outMsg->getId().data() << " because SendState= " << static_cast<int>(st);
                        continue; // skip this one
                    }
                    qCDebug(OcppComm) << "OcppOutMessageQueue: addReplaceExisting() item Id "
                                      << msg->getId().data() << " removed.";
                    itr = outMsgQ.erase(itr);
                    if (itr == outMsgQ.end())
                        break;
                }
            }
        }
        // append to out-queue
        qCDebug(OcppComm) << "OcppOutMessageQueue::addReplaceExisting " << outMsg->getId().c_str();

        this->add(outMsg);
    }
}

void OcppOutMessageQueue::onSendMessageTimeout()
{

    switch (st) {
    case SendState::Idle:
        qCWarning(OcppComm) << "OcppOutMessageQueue: unexpected State ("
                    << static_cast<int>(st) << ") receiving onSendMessageTimeout()";
        break;
    case SendState::Sent:
        qCWarning(OcppComm) << "OcppOutMessageQueue: timed-out waiting for send to complete";
        if (retryCnt < maxRetries) {
            st = SendState::Retrying;
            qCWarning(OcppComm) << "OcppOutMessageQueue: Retry sending for the " << retryCnt << "nth time.";
            startMsgSendRetryTimer();
        }
        else {
            QList<QSharedPointer<ocpp::RpcMessage>> &outMsgQ = (currentQ == MsgQueueType::NonTransactional)?
                        outNonTransMsgQ : outTransMsgQ;
            QSharedPointer<ocpp::RpcMessage> msg = outMsgQ.takeFirst();
//            emit msgError(MessageSendError::SendTimedOut, msg);
            st = SendState::Idle;
            scheduleNextIfQNotEmpty();
            retryCnt = 0;
        }
        break;
    case SendState::Retrying:
        retryCnt += 1;
        qCWarning(OcppComm) << "OcppOutMessageQueue: retry send again";
        sendMessage();
        break;
    case SendState::WaitReply:
    {
        qCWarning(OcppComm) << "OcppOutMessageQueue: timed-out waiting for Call Replies";
        QList<QSharedPointer<ocpp::RpcMessage>> &sentMsgQ = (currentQ == MsgQueueType::NonTransactional)? sentNonTransMsgQ : sentTransMsgQ;
        QSharedPointer<ocpp::RpcMessage> msg = sentMsgQ.first();
        std::string id = msg->getId();

        if (waitReplyRecordMap.contains(id) && waitReplyRecordMap[id] >= maxWaitReplyRetries)
        {
           qCWarning(OcppComm) << "retrWaitReplyCnt < maxWaitReplyRetries";
           waitReplyRecordMap.remove(id);
           emit msgNoReplyError(msg);
           st = SendState::Idle;
        }else{
           if(waitReplyRecordMap.contains(id)){
               waitReplyRecordMap[id] += 1;
           }else{
               waitReplyRecordMap.insert(id, 1);
           }
           restoreSentMsgToOutQ();
           st = SendState::Idle; // it should schedule send again.
           scheduleNextIfQNotEmpty();
       }
    }
        break;
    default:
        break;
    }
}

void OcppOutMessageQueue::scheduleNextIfQNotEmpty()
{
    if (csOnline && (!outNonTransMsgQ.empty() || !outTransMsgQ.empty()) && st == SendState::Idle) {
        qCDebug(OcppComm) << "OcppOutMessageQueue: scheduling processNextMessage with st = " << static_cast<int>(st);
        st = SendState::Scheduled; // prevent consecutive calls to this function, before the processingNextMessage is called.
        QTimer::singleShot(0, this, &OcppOutMessageQueue::processNextMessage);
    }
}

void OcppOutMessageQueue::sendMessage()
{
    // TBD: what happen if socket is invalid?

    qint64 numBytes = 0;
    try {
        QList<QSharedPointer<ocpp::RpcMessage>> &outMsgQ = (currentQ == MsgQueueType::NonTransactional)?
                    outNonTransMsgQ : outTransMsgQ;
        std::ostringstream oss;
        QSharedPointer<ocpp::RpcMessage> rpcMsg = outMsgQ.first();
        oss << *rpcMsg;
        //oss << outMsgQ.first();
        QString msgStr = QString::fromStdString(oss.str());
        // TBD: m_HeartBeatInterval =0;
        qCDebug(OcppComm) << "Message Send:" << msgStr;
        m_ChargerA.SendUdpLog(msgStr.toLatin1());
        startMsgSendToSocketTimer();
        st = SendState::Sent;
        numBytes = ws.sendTextMessage(msgStr);
        qCDebug(OcppComm) << "Bytes Sent: " << numBytes << "; Pending: " << ws.bytesToWrite();
    }
    catch (std::exception &e){
        qCWarning(OcppComm) << "Internal Error: Malformed Ocpp RpcMessage Object." << e.what();
        // Report Error?
    }
}

void OcppOutMessageQueue::onBytesWritten(qint64 numBytes)
{
    qint64 pendingCnt = ws.bytesToWrite();
    qCDebug(OcppComm) << "Bytes Written: " << numBytes << "; Pending: " << pendingCnt;
    if ((pendingCnt == 0) && (st == SendState::Sent)) {
        //qCWarning(OcppComm) << "OcppOutMessageQueue: message sent completed";
        stopSendMsgTimer();
        QList<QSharedPointer<ocpp::RpcMessage>> &outMsgQ = (currentQ == MsgQueueType::NonTransactional)?
                    outNonTransMsgQ : outTransMsgQ;
        QSharedPointer<ocpp::RpcMessage> msg = outMsgQ.takeFirst();
        emit msgSent(msg);
        if (msg.get()->getType() == ocpp::RpcMessageType::Call) {
            QList<QSharedPointer<ocpp::RpcMessage>> &sentMsgQ = (currentQ == MsgQueueType::NonTransactional)?
                        sentNonTransMsgQ : sentTransMsgQ;
            sentMsgQ.append(msg);
            st = SendState::WaitReply;
            startNoReplyTimer();
        }
        else {
            qCDebug(OcppComm) << "OcppOutMessageQueue: onBytesWritten() sent message is not Call-Type";
            st = SendState::Idle;
            scheduleNextIfQNotEmpty();
        }
        retryCnt = 0; // reset;
    }
}

void OcppOutMessageQueue::restoreSentMsgToOutQ()
{
    QList<QSharedPointer<ocpp::RpcMessage>> &sentMsgQ = (currentQ == MsgQueueType::NonTransactional)?
                sentNonTransMsgQ : sentTransMsgQ;
    QList<QSharedPointer<ocpp::RpcMessage>> &outMsgQ = (currentQ == MsgQueueType::NonTransactional)?
                outNonTransMsgQ : outTransMsgQ;
    while (!sentMsgQ.empty()) {
        QSharedPointer<ocpp::RpcMessage> msg = sentMsgQ.takeLast();
        outMsgQ.push_front(msg);
        qCDebug(OcppComm) << "Restoring Sent message of UID " << msg->getId().c_str() << " to out-queue";
    }
}

void OcppOutMessageQueue::processNextMessage()
{
    st = SendState::Idle;
    if (!outNonTransMsgQ.empty()) {
        currentQ = MsgQueueType::NonTransactional;
        sendMessage();
    }
    else if (!outTransMsgQ.empty()) {
        currentQ = MsgQueueType::Transactional;
        sendMessage();
    }
}

QSharedPointer<ocpp::RpcMessage> OcppOutMessageQueue::removeSentCallMessage(const QString &id)
{
    QSharedPointer<ocpp::RpcMessage> callMsg;
    QList<QSharedPointer<ocpp::RpcMessage>> &sentMsgQ = (currentQ == MsgQueueType::NonTransactional)?
                sentNonTransMsgQ : sentTransMsgQ;
    for (auto itr = sentMsgQ.begin(); itr != sentMsgQ.end(); itr++) {
        if (itr->get()->getId().compare(id.toStdString()) == 0) {
            qCDebug(OcppComm) << "OcppOutMessageQueue: found corresponding Call-Message("
                                << id << ")";
            callMsg = *itr;
            itr = sentMsgQ.erase(itr);
            if (itr == sentMsgQ.end())
                break;
        }
    }
    if (!callMsg.isNull() && (st == SendState::WaitReply)) {
        stopSendMsgTimer();
        st = SendState::Idle;
        scheduleNextIfQNotEmpty();
    }

    return callMsg;
}

void OcppOutMessageQueue::removeRealTimeMsgFromQ(QList<QSharedPointer<ocpp::RpcMessage>> &msgQ)
{
    for (auto itr = msgQ.begin(); itr != msgQ.end(); itr++) {
        if (itr->get()->getType() == ocpp::RpcMessageType::Call) {
            QSharedPointer<ocpp::CallMessage> callMsg = itr->dynamicCast<ocpp::CallMessage>();
            if (callMsg->getAction() == ocpp::callActionStrs.at(ocpp::CallActionType::Authorize)) {
                qDebug() << "Server is offline. Removing ocpp::Authorize request from outgoing queues. msgId=" << callMsg->getId().data();
                emit msgError(MessageSendError::ServerOffline, *itr);
                itr = msgQ.erase(itr);
                if (itr == msgQ.end())
                    break;
            }
        }
    }
}

void OcppOutMessageQueue::onWsStateChanged(QAbstractSocket::SocketState sockState)
{
    switch (sockState) {
    case QAbstractSocket::SocketState::ConnectedState:
        qCDebug(OcppComm) << "WebSocket Connected: OcppOutMessageQueue start processing messages";
        csOnline = true;
        scheduleNextIfQNotEmpty();
        retryCnt = 0; // reset;
        break;
    case QAbstractSocket::SocketState::ConnectingState:
    case QAbstractSocket::SocketState::UnconnectedState:
        qCDebug(OcppComm) << "WebSocket Not Connected: OcppOutMessageQueue stop processing messages";
        csOnline = false;
        removeRealTimeMsgFromQ(outNonTransMsgQ);
        removeRealTimeMsgFromQ(sentNonTransMsgQ);
        sentNonTransMsgQ.clear(); // Keep transactional messages in sent queue?
        restoreSentMsgToOutQ();
        sendMsgTimer.stop();
        st = SendState::Idle;
        retryCnt = 0; // reset;
        break;
    default:
        break;
    }
}
