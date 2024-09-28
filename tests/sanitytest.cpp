#include "sanitytest.h"
#include "hostplatform.h"
#include "chargerconfig.h"
#include "logconfig.h"
#include <QApplication>
#include <QDebug>
#include <iostream>

SanityTest::SanityTest(QObject *parent) : QObject(parent)
{

}

void SanityTest::start()
{
    // Perform the following tests then quit
    //  try open port to charger-board comport
    qWarning() << "Sanity test start.";
    std::cout <<"Sanity test start.";
    chargerSerPortTest();
    //  try open RFID port if enabled
    if (charger::getRfidSupported())
        rfidSerPortTest();
    else
        results.at(TestType::RfidPort) = TestStatus::Disabled;
    //  connect to OCPP CS listed in settings.ini
    ocppConnTest();
    //distSensorThread.StartSlave("/dev/ttyAMA0",200);
    std::cout <<"Sanity test end.";
}

void SanityTest::chargerSerPortTest()
{
    qCDebug(::SanityTest) << "Testing Charger-Board Connection";
    connect(&chargerPortThread, &CComPortThread::portOpened, this, &SanityTest::onChargerPortOpened);
    results.at(TestType::ChargerPort) = TestStatus::Started;
    chargerPortThread.StartSlave(5000);
}

void SanityTest::onChargerPortOpened(bool succ)
{
    qWarning() << "Sanity test. Charger-Board Connection Test " << (succ? "Passed" : "Failed");
    qCDebug(::SanityTest) << "Charger-Board Connection Test " << (succ? "Passed" : "Failed");
    if (succ)
        results.at(TestType::ChargerPort) = TestStatus::Passed;
    else
        results.at(TestType::ChargerPort) = TestStatus::Failed;
    checkAllTestsCompleted();
}

void SanityTest::rfidSerPortTest()
{
    qCDebug(::SanityTest) << "Testing RFID-Reader Module";
    connect(&rfidReaderThread, &CRfidThread::readerPortOpened, this, &SanityTest::onRfidReaderPortOpened);
    connect(&rfidReaderThread, &CRfidThread::readerModuleActivated, this, &SanityTest::onRfidModuleActivated);
    results.at(TestType::RfidPort) = TestStatus::Started;
    rfidReaderThread.StartSlave(charger::Platform::instance().rfidSerialPortName, 500);
}

void SanityTest::onRfidReaderPortOpened(bool succ)
{
    qWarning() << "Sanity test. RFID Reader Connection Test " << (succ? "Passed" : "Failed");
    qCDebug(::SanityTest) << "RFID Reader Connection Test " << (succ? "Passed" : "Failed");
    if (!succ) {
        results.at(TestType::RfidPort) = TestStatus::Failed;
        checkAllTestsCompleted();
    }
}

void SanityTest::onRfidModuleActivated(bool succ)
{
    qWarning() << "Sanity test. RFID Reader Activation Test " << (succ? "Passed" : "Failed");
    qCDebug(::SanityTest) << "RFID Reader Activation Test " << (succ? "Passed" : "Failed");
    if (succ)
        results.at(TestType::RfidPort) = TestStatus::Passed;
    else
        results.at(TestType::RfidPort) = TestStatus::Failed;

    checkAllTestsCompleted();
}

void SanityTest::ocppConnTest()
{
    qCDebug(::SanityTest) << "Testing Ocpp-CS connection";
    results.at(TestType::OcppConn) = TestStatus::Started;
    connect(&ws, &QWebSocket::connected, this, &SanityTest::onWsConnected);
    connect(&ws, &QWebSocket::disconnected, this, &SanityTest::onWsDisconnected);
    connect(&ws, &QWebSocket::stateChanged, this, &SanityTest::onWsStateChanged);
    connect(&ws, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &SanityTest::onWsError);

    typedef void (QWebSocket:: *sslErrorsSignal)(const QList<QSslError> &);
    connect(&ws, static_cast<sslErrorsSignal>(&QWebSocket::sslErrors), this, &SanityTest::onSslErrors);
    qWarning() << "Sanity test. WS connection to  " << charger::getOcppCsUrl();
    ws.ignoreSslErrors();
    ws.open(QUrl(charger::getOcppCsUrl()));

}

void SanityTest::onWsConnected()
{
    qWarning() << "Sanity test. WS connected. Passed";
    qCDebug(::SanityTest) << "Ocpp-CS Connection Test Passed";
    results.at(TestType::OcppConn) = TestStatus::Passed;
    checkAllTestsCompleted();
}

void SanityTest::onWsDisconnected()
{
    qWarning() << "Sanity test. WS disconnected. Failed";
    qCDebug(::SanityTest) << "Ocpp-CS Connection Test Failed" << " (Disconnected)" << ws.error();
    results.at(TestType::OcppConn) = TestStatus::Failed;
    checkAllTestsCompleted();
}

void SanityTest::onWsStateChanged(QAbstractSocket::SocketState sockState)
{
    qWarning() << "Sanity test. WS state changed. State: " << sockState;
    Q_UNUSED(sockState);
    qCDebug(::SanityTest) << "Ocpp-CS Connection Test " << " State " << sockState;
}

void SanityTest::onWsError(QAbstractSocket::SocketError sockError)
{
    qWarning() << "Sanity test. WS error. Error: " << ws.error();
    Q_UNUSED(sockError);
    qCDebug(::SanityTest) << "Ocpp-CS Connection Test Failed" << ws.error();
    results.at(TestType::OcppConn) = TestStatus::Failed;
    checkAllTestsCompleted();
}

void SanityTest::onSslErrors(const QList<QSslError> &errors)
{
    qWarning() << "Sanity test. SSL Errors received:";
    for (auto err : errors) {
        qWarning(OcppComm) << err.errorString();
    }

    // WARNING: Never ignore SSL errors in production code.
    // The proper way to handle self-signed certificates is to add a custom root
    // to the CA store.

    ws.ignoreSslErrors();
}

void SanityTest::checkAllTestsCompleted()
{
    bool incomplete = false;
    bool allTestPassed = true;

    for (auto p: results) {
        switch (p.second) {
        case TestStatus::Failed:
            allTestPassed = false;
            break;
        case TestStatus::Started:
        case TestStatus::NotTested:
            incomplete = true;
            break;
        default:
            break;
        }
    }
    if (incomplete)
        return;
    qWarning() << "Sanity test. Result: allTestPassed? "<< allTestPassed;
    QApplication::exit(allTestPassed? 0 : -2);
}
