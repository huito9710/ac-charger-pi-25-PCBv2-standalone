#include "hostplatform.h"

#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QApplication>
#include <QProcess>
#include <QtGlobal>
#include <QDateTime>
#include <syslog.h>

namespace charger {

/*
 * Local Variables (this file only)
 */
constexpr char progDir[] = "Charger";
constexpr char imagesDir[] = "JPG";
constexpr char settingsDir[] = "upfiles";
constexpr char recsDir[] = "Downloads";
constexpr char dbDir[] = "upfiles";
constexpr char userDldsDir[] = "Downloads";
constexpr char sshDir[] = ".ssh";
constexpr char sshConfigFile[] = "config";
constexpr char runDataDir[] = "upfiles";
constexpr char firmwareUpdateScript[] = "evmega_firmware_update.py";
constexpr char firmwareUpdateReadyFile[] = "install_package_ready";
constexpr char firmwareUpdateResultFile[] = "install_result";
constexpr char versionFile[] = "VERSION.txt";
/*
 * Class Static Variables
 */
Platform::Platform()
    : guiImageFileBaseDir(QDir::fromNativeSeparators(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(progDir).arg(imagesDir)))),
      defaultWinState(Qt::WindowFullScreen),
      hideCursor(true),
      settingsIniDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(settingsDir))),
      passwordDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(settingsDir))),
      rfidCardsDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(settingsDir))),
      recordsDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(recsDir))),
      videosDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(settingsDir))),
      rfidSerialPortName("pn532_i2c:/dev/i2c-1"),
      localAuthListDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(dbDir))),
      authCacheDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(dbDir))),
      downloadsDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(userDldsDir))),
      sshConfigFilePath(QDir::fromNativeSeparators(QString("%1%2%3%2%4").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(sshDir).arg(sshConfigFile))),
      firmwareStatusDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(runDataDir))),
      firmwareUpdateScriptFilePath(QDir::fromNativeSeparators(QDir::fromNativeSeparators(QString("%1%2%3%2%4").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(progDir).arg(firmwareUpdateScript)))),
      firmwareInstallReadyFilePath(QDir::fromNativeSeparators(QDir::fromNativeSeparators(QString("%1%2%3%2%4").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(progDir).arg(firmwareUpdateReadyFile)))),
      firmwareInstallResultFilePath(QDir::fromNativeSeparators(QDir::fromNativeSeparators(QString("%1%2%3%2%4").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(progDir).arg(firmwareUpdateResultFile)))),
      firmwareVerFilePath(QDir::fromNativeSeparators(QDir::fromNativeSeparators(QString("%1%2%3%2%4").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(progDir).arg(versionFile)))),
      connAvailabilityInfoDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(runDataDir))),
      translationFileDir(QDir::fromNativeSeparators(QString("%1%2%3%2").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).arg(QDir::separator()).arg(settingsDir)))
{}

void Platform::scheduleReboot()
{
    qCritical() << "ScheduleReboot... Bye....";
    // TODO: schedule shutdown after 10 seconds?
    QProcess::startDetached("shutdown -r +1");
    // Quit the application now to avoid other complications.
    QApplication::quit();
}

static void qtMsgToSyslog(QtMsgType qMsgType, const QMessageLogContext & ctxt, const QString &msg)
{
    Q_UNUSED(ctxt);

    static std::map<QtMsgType, int> priorities {
      { QtDebugMsg, LOG_DEBUG },
        {QtInfoMsg, LOG_INFO },
        {QtWarningMsg, LOG_WARNING},
        {QtCriticalMsg, LOG_CRIT},
        {QtFatalMsg, LOG_EMERG},
        {QtSystemMsg, LOG_CRIT}
    };

    syslog(priorities.at(qMsgType), msg.toUtf8());
}

void Platform::setupLogChannels()
{
    openlog("charger", LOG_CONS, LOG_USER);
    qInstallMessageHandler(qtMsgToSyslog);
}

void Platform::updateSystemTime(const QDateTime currTime)
{
    // Nothing for now
    QString localTimeStr = currTime.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
    qDebug() << "Requested to Update System Time to " << localTimeStr;
    // sudo timedatectl set-time "YY-MM-DD HH:MM:SS" (Local time)
    QString program = "bash";
    QStringList args;
    args.append("-c");
    args.append(QString("sudo timedatectl set-time \"%1\"").arg(localTimeStr));
    qDebug() << program << " " << args.join(' ');
    QProcess::startDetached(program, args);
}

void Platform::hardreset()
{
    QProcess::startDetached("shutdown -r +1");
}

void Platform::asyncShellCommand(QStringList strlist,QString &output){
    QString out;
    QProcess * process = new QProcess();
    process->setReadChannel(QProcess::StandardOutput);
    process->setWorkingDirectory("/home/pi/Desktop");
    QObject ::connect(process,&QProcess::readyReadStandardOutput,[process,&output](){
        output = QString(process->readAllStandardOutput());
    });
    process->start("/bin/sh", strlist);

    bool fi = process->waitForFinished();
    if(fi){
        process->close();
        delete process;
    }
}
void Platform::executeCommand(QStringList strlist){
    QString out;
    QProcess * process = new QProcess();
    process->setReadChannel(QProcess::StandardOutput);
    process->setWorkingDirectory("/home/pi/Desktop");

    process->execute("/bin/sh", strlist);

    bool fi = process->waitForFinished();
    if(fi){
        //output = QString(process->readAllStandardOutput());
        process->close();
        delete process;
    }
}

}
