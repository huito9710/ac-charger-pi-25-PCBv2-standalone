#include "cocppthread.h"
#include "ocppclient.h"

COcppThread::COcppThread(QObject *parent) :
    QThread(parent)
{
    //stop      =false;
    reconnect =false;
}

void COcppThread::stop()
{
    m_pclient->CloseWebSocket();
    this->quit();
}

void COcppThread::run()
{
//    //OcppClient client(QUrl(QStringLiteral("ws://192.168.168.101:8080/ocpp/GLTEST1")));
//    OcppClient client;
//    m_pclient =&client;
//    m_pclient->ConnectServer();
//    //Q_UNUSED(client);

    //this->ws.open(QUrl(QStringLiteral("ws://192.168.168.101:8080/ocpp/GLTEST2")));

    this->exec();
}
