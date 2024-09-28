#ifndef FIRMWAREUPDATESCHEDULER_H
#define FIRMWAREUPDATESCHEDULER_H

#include <QObject>
#include <QProcess>
#include "softwaredownloader.h"
#include "firmwaredownloadstate.h"

class FirmwareUpdateScheduler : public QObject
{
    Q_OBJECT
public:
    static FirmwareUpdateScheduler &getInstance() {
        static FirmwareUpdateScheduler theFuScheduler;
        return theFuScheduler;
    }

    bool newUpdateRequest(const QString &uri, const QDateTime availDt);
    // Either numRetries or retryIntervalSecs can be given on its own
    // Negative-value will be ignored.
    bool newUpdateRequest(const QString &uri, const QDateTime availDt,
                          int numRetries, int retryIntervalSecs);
    bool checkInstallStatus();

signals:
    void downloadStarted();
    void downloadReady(const QString &filePath);
    void downloadFailed(const QString &errStr);
    void installReady(const QString &filePath);
    void installFailed(const QString &errStr);
    void installCompleted();
    void installNone(); // There was no package to install.

private slots:
    void onFirmwareDownloadCompleted(QString &localPath);
    void onFirmwareDownloadFailed(QString &errStr);
    void onDownldSchTimeout();
    void onCheckDownloadedFile();
    void onProcError(QProcess::ProcessError error);
    void onProcFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcStdoutReady();
    void onProcStderrReady();
    void onCheckInstallResult();

private:
    explicit FirmwareUpdateScheduler(QObject *parent = nullptr);
    bool saveStateToFile();
    bool loadStateFromFile();
    void scheduleNewDownload();

    SoftwareDownloader swDldr;
    QTimer downldSchTimer;
    struct FirmwareDownloadState downldSt;
    static constexpr uint32_t magicNumber = 0x24292160;
    ////////////////////////////////////////////////////////////////////////////////
    // Developer Note: must update both versions at the same time in the future
    static constexpr uint32_t verNumber = 0x512;
    static constexpr QDataStream::Version datastreamVer = QDataStream::Version::Qt_5_12;
    ////////////////////////////////////////////////////////////////////////////////
    const QString fuStateFilePath;
    QProcess fwupdateProc;
};

#endif // FIRMWAREUPDATESCHEDULER_H
