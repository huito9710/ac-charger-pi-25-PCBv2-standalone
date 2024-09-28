#include "hostplatform.h"

#include <QDir>
#include <QStandardPaths>
#include <QApplication>
#include <QDebug>
#include <QDateTime>

namespace charger {

/*
 * Local Variables (this file only)
 */
constexpr char progDir[] = "Charger.native_win64";
constexpr char imagesDir[] = "Images";
constexpr char settingsDir[] = "Settings";
constexpr char recsDir[] = "Records";
constexpr char vidsDir[] = "Videos";
constexpr char dbDir[] = "Databases";
constexpr char userDldsDir[] = "Downloads";
constexpr char sshDir[] = "ssh";
constexpr char sshConfigFile[] = "config";
constexpr char runDataDir[] = "run";
constexpr char versionFile[] = "VERSION.txt";

Platform::Platform()
    : guiImageFileBaseDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(imagesDir))),
      defaultWinState(Qt::WindowActive),
      hideCursor(false),
      settingsIniDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(settingsDir))),
      passwordDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(settingsDir))),
      rfidCardsDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(settingsDir))),
      recordsDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(recsDir))),
      videosDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(vidsDir))),
      rfidSerialPortName("COM7"),
      localAuthListDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(dbDir))),
      authCacheDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(dbDir))),
      downloadsDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(userDldsDir))),
      sshConfigFilePath(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2%5").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(sshDir).arg(sshConfigFile))),
      firmwareStatusDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(runDataDir))),
      firmwareVerFilePath(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2%5").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(runDataDir).arg(versionFile))),
      connAvailabilityInfoDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(runDataDir))),
      translationFileDir(QDir::fromNativeSeparators(QString("%1%2%3%2%4%2").arg(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).arg(QDir::separator()).arg(progDir).arg(settingsDir)))
{}


void Platform::scheduleReboot()
{
    qDebug() << "ScheduleReboot... Bye....";
    QApplication::quit();
}

void Platform::setupLogChannels()
{
    // Nothing for now.
}

void Platform::updateSystemTime(const QDateTime currTime)
{
    // Nothing for now
    qDebug() << "Requested to Update System Time to " << currTime.toLocalTime().toString(Qt::DateFormat::LocalDate);
}
}
