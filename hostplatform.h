#ifndef HOSTPLATFORM_H
#define HOSTPLATFORM_H

#include <QString>
#include <QWindow>

namespace charger {

class Platform {
public:
    static Platform &instance() { static Platform platform; return platform; };
    // Directory of storing the images used in GUI.
    const QString guiImageFileBaseDir;
    // Default Window-State
    const Qt::WindowState defaultWinState;
    // Hide Cursor for non-mouse systems
    const bool hideCursor;
    // Directory of settings.ini
    const QString settingsIniDir;
    // Directory of password.lst
    const QString passwordDir;
    // Directory of Card.lst
    const QString rfidCardsDir;
    // Directory of Record Files
    const QString recordsDir;
    // Directory of Video Files
    const QString videosDir;
    // RFID UART port
    const QString rfidSerialPortName;
    // Authorization List location
    const QString localAuthListDir;
    // Authorization Cache location
    const QString authCacheDir;
    // Download directory
    const QString downloadsDir;
    // SSH Config file path
    const QString sshConfigFilePath;
    // Directory of Firmware-Update program-state information
    const QString firmwareStatusDir;
    // firmware-update-script location
    const QString firmwareUpdateScriptFilePath;
    // firmware-update install-ready-file location
    const QString firmwareInstallReadyFilePath;
    // firmware-update install-result-file location
    const QString firmwareInstallResultFilePath;
    // software version file location
    const QString firmwareVerFilePath;
    // connector-availability (persist across reboot)
    const QString connAvailabilityInfoDir;
    // Translation file directory
    const QString translationFileDir;

    // Schedule Reboot system
    void scheduleReboot();

    // redirect output console messages to files here
    void setupLogChannels();

    // update system time
    void updateSystemTime(const QDateTime currTime);

    //hardreset
    void hardreset();

    static void asyncShellCommand(QStringList strlist, QString &ouput);
    static void detachedShellCommand(QStringList strlist);
    static void executeCommand(QStringList strlist);

private:
    Platform();
};

}

#endif // HOSTPLATFORM_H
