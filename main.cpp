#include "mainwindow.h"
#include <QApplication>
#include <QLoggingCategory>
#include <QTranslator>
#include "hostplatform.h"
#include "chargerconfig.h"
#include "tests/sanitytest.h"

// QT Log Category Definitions
Q_LOGGING_CATEGORY(ChargerComm, "ChargerComm", QtWarningMsg);
Q_LOGGING_CATEGORY(ChargerMessages, "ChargerMessages", QtWarningMsg);
Q_LOGGING_CATEGORY(OcppComm, "OcppComm", QtWarningMsg);
Q_LOGGING_CATEGORY(AuthCache, "AuthCache", QtWarningMsg);
Q_LOGGING_CATEGORY(FirmwareUpdate, "FirmwareUpdate", QtWarningMsg);
Q_LOGGING_CATEGORY(SanityTest, "SanityTest", QtInfoMsg);
Q_LOGGING_CATEGORY(Rfid, "Rfid", QtWarningMsg);
Q_LOGGING_CATEGORY(Transactions, "EvTransactions", QtWarningMsg);

int main(int argc, char *argv[])
{
//    if(system("gpio export 18 out \n") ==-1)
//        return 0;   //外部设置GPIO

    charger::Platform::instance().setupLogChannels();

    QApplication a(argc, argv);

    if ((argc > 1) && strncmp(argv[1], "--sanity-test", 20) == 0)
    {
        qWarning() << "Main: Start sanity test";
        class SanityTest test(nullptr);

        test.start();

        return a.exec();
    }

    charger::initChargerConfig();

    MainWindow w;

    qWarning() << "Main: Start main window";
    if (charger::Platform::instance().hideCursor)
        QApplication::setOverrideCursor(Qt::BlankCursor);
    w.setWindowState(charger::Platform::instance().defaultWinState);
    w.show();

    //Tester t;

    return a.exec();
}
