#ifndef SANITYTEST_H
#define SANITYTEST_H

#include <QObject>
#include <QWebSocket>
#include <map>
#include "ccomportthread.h"
#include "crfidthread.h"
#include "distancesensorthread.h"

class SanityTest : public QObject
{
    Q_OBJECT
public:
    explicit SanityTest(QObject *parent = nullptr);

    void start();

signals:

private slots:
    void onChargerPortOpened(bool);
    void onRfidReaderPortOpened(bool);
    void onRfidModuleActivated(bool);
    void onWsConnected();
    void onWsDisconnected();
    void onWsError(QAbstractSocket::SocketError error);
    void onSslErrors(const QList<QSslError> &errors);
    void onWsStateChanged(QAbstractSocket::SocketState sockState);

private:
    void chargerSerPortTest();
    void rfidSerPortTest();
    void ocppConnTest();
    void checkAllTestsCompleted();
    enum class TestType : int {
        ChargerPort,
        RfidPort,
        OcppConn
    };
    enum class TestStatus : int {
        Disabled,
        NotTested,
        Started,
        Failed,
        Passed,
    };
    std::map<TestType, TestStatus> results = {
        {TestType::ChargerPort, TestStatus::NotTested },
        {TestType::RfidPort, TestStatus::NotTested },
        {TestType::OcppConn, TestStatus::NotTested }
    };

    DistanceSensorThread distSensorThread;
    CComPortThread chargerPortThread;
    CRfidThread rfidReaderThread;
    QWebSocket ws;
};

#endif // SANITYTEST_H

