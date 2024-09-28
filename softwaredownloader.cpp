#include "softwaredownloader.h"
#include "hostplatform.h"
#include "logconfig.h"
#include <QDebug>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>

SoftwareDownloader::SoftwareDownloader() :
    timeout(30*60),
    localDir(charger::Platform::instance().downloadsDir)
{
    procTimer.setSingleShot(true);
    procTimer.setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
    connect(&procTimer, &QTimer::timeout, this, &SoftwareDownloader::onProcTimerExpired);

    connect(&downloadProc, &QProcess::errorOccurred, this, &SoftwareDownloader::onProcError);
    connect(&downloadProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SoftwareDownloader::onProcFinished);
    connect(&downloadProc, &QProcess::readyReadStandardError, this, &SoftwareDownloader::onProcStderrReady);
    connect(&downloadProc, &QProcess::readyReadStandardOutput, this, &SoftwareDownloader::onProcStdoutReady);
}

void SoftwareDownloader::onProcTimerExpired()
{
    switch (downloadProc.state()) {
    case QProcess::ProcessState::Starting:
    case QProcess::ProcessState::Running:
    {
        QString errStr = QString("SoftwareDownloader timeout detected on current process(%1).").arg(downloadProc.processId());
        qCWarning(FirmwareUpdate) << errStr;
        downloadProc.close();
        emit downloadFailed(errStr);
    }
        break;
    default:
        break;
    }
}

void SoftwareDownloader::onProcError(QProcess::ProcessError error)
{
    QString errStr;
    switch (error) {
    case QProcess::ProcessError::FailedToStart:
        errStr = QString("SoftwareDownloader error: %1 failed to start.").arg(downloadProc.program());
        qCWarning(FirmwareUpdate) << errStr;
        emit downloadFailed(errStr); // do not expect finish() signal
        break;
    case QProcess::ProcessError::Crashed:
        errStr = QString("SoftwareDownloader error: %1 crashed.").arg(downloadProc.program());
        qCWarning(FirmwareUpdate) << errStr;
        break;
    case QProcess::ProcessError::UnknownError:
        errStr = QString("SoftwareDownloader error: %1 has unknown error.").arg(downloadProc.program());
        qCWarning(FirmwareUpdate) << errStr;
    default:
        // Has the program died yet? Wait for the 'finish' callback?
        break;
    }
}
void SoftwareDownloader::onProcFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString errStr;
    switch (exitStatus) {
    case QProcess::ExitStatus::NormalExit:
        // exitCode cannot be relied upon (at least on Windows). // Check the downloaded file instead
        if (QFile::exists(localFilePath)) {
            emit downloadCompleted(localFilePath);
        }
        else {
            errStr = QString("SoftwareDownloader finished: download file not exist, assumed failed. Exit-code(%1); %2").arg(exitCode)
                    .arg(downloadProc.errorString());
            emit downloadFailed(errStr);
        }
        break;
    case QProcess::ExitStatus::CrashExit:
        errStr = QString("SoftwareDownloader finished: %1 crashed.").arg(downloadProc.program());
        emit downloadFailed(errStr);
        break;
    }
}

void SoftwareDownloader::onProcStdoutReady()
{
    QByteArray stdoutBuf = downloadProc.readAllStandardOutput();
    qCWarning(FirmwareUpdate) << stdoutBuf.toStdString().c_str();
}

void SoftwareDownloader::onProcStderrReady()
{
    QByteArray stderrBuf = downloadProc.readAllStandardError();
    qCCritical(FirmwareUpdate) << stderrBuf.toStdString().c_str();
}

bool SoftwareDownloader::Download(const QString &uri)
{
    bool accepted = false;
    // Assuming the uri is http://<host>:<port>/<remote-file-path>
    QUrl decodedUrl(uri);
    try {
        if (!decodedUrl.isValid())
            throw QString("SoftwareDownloader Cannot understand the download link %1 Error: %2")
                .arg(uri).arg(decodedUrl.errorString());
        if (decodedUrl.scheme() != "http" && decodedUrl.scheme() != "https")
            throw QString("SoftwareDownloader expects link starts with HTTP or HTTPS.");
        QString remoteFileName = decodedUrl.fileName();
        if (remoteFileName.isNull() || remoteFileName.isEmpty())
            throw QString("SoftwareDownloader expects a file in the link");
        if (downloadProc.state() != QProcess::ProcessState::NotRunning)
            throw QString("SoftwareDownloader: Process is still starting/running");
        // Prepare process to 'start'
        localFilePath = QDir::fromNativeSeparators(QString("%1%2").arg(charger::Platform::instance().downloadsDir).arg(remoteFileName));
        if (QFile::exists(localFilePath)) {
            if (QFile::remove(localFilePath))
                qCDebug(FirmwareUpdate) << "SoftwareDownloader Removing file successfully before download.";
            else
                qCWarning(FirmwareUpdate) << "SoftwareDownloader Failed to remove file before download. " << localFilePath;
        }

        QStringList args;
        args << "--insecure" << "-sS" << "-iL" << "-f" << uri << "-o" << localFilePath;

        qCDebug(FirmwareUpdate) << args;
        downloadProc.setProgram("curl");
        downloadProc.setArguments(args);
        downloadProc.start(QIODevice::ReadOnly);
        accepted = true;
    }
    catch (QString &errStr)
    {
        qCWarning(FirmwareUpdate) << errStr;
        accepted = false;
    }
    return accepted;
}

bool SoftwareDownloader::StopDownload()
{
    if (downloadProc.state() != QProcess::ProcessState::NotRunning)
        downloadProc.close();
    return true;
}
