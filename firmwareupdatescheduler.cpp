#include "firmwareupdatescheduler.h"
#include "hostplatform.h"
#include "logconfig.h"
#include <QDateTime>
#include <QTimer>
#include <QDataStream>
#include <QFile>
#include <QDebug>

FirmwareUpdateScheduler::FirmwareUpdateScheduler(QObject *parent) : QObject(parent),
    fuStateFilePath(QString("%1firmware_update.dat").arg(charger::Platform::instance().firmwareStatusDir))
{
    // connect signals
    connect(&downldSchTimer, &QTimer::timeout, this, &FirmwareUpdateScheduler::onDownldSchTimeout);
    connect(&swDldr, &SoftwareDownloader::downloadCompleted, this, &FirmwareUpdateScheduler::onFirmwareDownloadCompleted);
    connect(&swDldr, &SoftwareDownloader::downloadFailed, this, &FirmwareUpdateScheduler::onFirmwareDownloadFailed);

    connect(&fwupdateProc, &QProcess::errorOccurred, this, &FirmwareUpdateScheduler::onProcError);
    connect(&fwupdateProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &FirmwareUpdateScheduler::onProcFinished);
    connect(&fwupdateProc, &QProcess::readyReadStandardError, this, &FirmwareUpdateScheduler::onProcStderrReady);
    connect(&fwupdateProc, &QProcess::readyReadStandardOutput, this, &FirmwareUpdateScheduler::onProcStdoutReady);

    // load state from file
    loadStateFromFile();
    // set up timer if needed
    if (!downldSt.downloaded)
        scheduleNewDownload();
}

bool FirmwareUpdateScheduler::newUpdateRequest(const QString &uri, const QDateTime availDt)
{
    return this->newUpdateRequest(uri, availDt, -1, -1);
}

bool FirmwareUpdateScheduler::newUpdateRequest(const QString &uri, const QDateTime availDt,
                                               int numRetries, int retryIntervalSecs)
{
    qCDebug(FirmwareUpdate) << "Received firmware-update request: "
                            << " location: " << uri
                            << " available-time: " << availDt.toLocalTime()
                            << " Num-of-Retries: " << numRetries
                            << " Interval-between-Retries(sec): " << retryIntervalSecs;
    // stop ongoing download if needed.
    if (downldSt.downloaded == false)
        swDldr.StopDownload();
    // replace the FirmwareDownloadState value with this information
    downldSt.downloaded = false;
    downldSt.downloadUri = uri;
    downldSt.availDateTime = availDt;
    downldSt.retries = (numRetries < 0)? FirmwareDownloadState::defaultNumDownloadRetries : numRetries;
    downldSt.remainAttempts = downldSt.retries;
    downldSt.retryInterval = (retryIntervalSecs < 0)? FirmwareDownloadState::defaultDownloadRetryInterval
                                                    : std::chrono::seconds(retryIntervalSecs);
    // save state
    saveStateToFile();
    scheduleNewDownload();

    return true;
}

bool FirmwareUpdateScheduler::checkInstallStatus()
{
    QTimer::singleShot(0, this, &FirmwareUpdateScheduler::onCheckInstallResult);
    return true;
}

void FirmwareUpdateScheduler::onCheckInstallResult()
{
    auto resFilePath = charger::Platform::instance().firmwareInstallResultFilePath;
    if (QFile::exists(resFilePath)) {
        // Read the first line
        QFile resFile(resFilePath);
        if(resFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
            QByteArray firstLine = resFile.readLine().trimmed();
            if (firstLine.toUpper() == "OK") {
                emit installCompleted();
            }
            else {
                QByteArray remainText = resFile.readAll();
                emit installFailed(remainText);
            }
            resFile.close();
        }
        // remove file
        QFile::remove(resFilePath);
    }
    else {
        emit installNone();
    }
}


void FirmwareUpdateScheduler::scheduleNewDownload()
{
    Q_ASSERT(downldSt.downloaded == false);

    if (QDateTime::currentDateTime() >= downldSt.availDateTime) {
        // schedule ASAP
        downldSchTimer.setSingleShot(true);
        downldSchTimer.setInterval(0); // ASAP
        downldSchTimer.start();
    }
    else {
        // schedule a minute-timer until the time comes
        downldSchTimer.setSingleShot(false);
        downldSchTimer.setInterval(1000 * 60); // minute
        downldSchTimer.start();
    }
}


void FirmwareUpdateScheduler::onDownldSchTimeout()
{
    Q_ASSERT(downldSt.downloaded == false);
    // start download if avail-time is passed
    if (QDateTime::currentDateTime() >= downldSt.availDateTime) {
        // stop timer
        downldSchTimer.stop();
        qCDebug(FirmwareUpdate) << "FirmwareUpdateScheduler make a download attempt";
        bool isFirstAttempt = (downldSt.remainAttempts == downldSt.retries); // first-time
        downldSt.remainAttempts -= 1;
        if (swDldr.Download(downldSt.downloadUri)) {
            // signal downloadStarted() if it is the first-time
            if (isFirstAttempt) // first-time
                emit downloadStarted();
        }
        else {
            if (downldSt.remainAttempts > 0) {
                downldSchTimer.setSingleShot(true);
                downldSchTimer.setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(downldSt.retryInterval));
                downldSchTimer.start();
                qCDebug(FirmwareUpdate) << "FirmwareUpdateScheduler scheduled another download attempt";
            }
            else {
                downldSt.downloaded = true; // to prevent further attempt
                emit downloadFailed("FirmwareUpdateScheduler Unable to Start download");
            }
        }
        saveStateToFile();
    }
}

void FirmwareUpdateScheduler::onFirmwareDownloadFailed(QString &errStr)
{
    // update attempt
    // schedule another download if necessary
    // save state
    // signal downloadFailed if all attempts completed.
    if (downldSt.remainAttempts > 0) {
        downldSchTimer.setSingleShot(true);
        downldSchTimer.setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(downldSt.retryInterval));
        downldSchTimer.start();
        qCDebug(FirmwareUpdate) << "FirmwareUpdateScheduler scheduled another download attempt";
    }
    else {
        downldSt.downloaded = true; // to prevent further attempt
        saveStateToFile();
        emit downloadFailed(errStr);
    }
}

void FirmwareUpdateScheduler::onFirmwareDownloadCompleted(QString &localPath)
{
    // update state
    downldSt.downloaded = true;
    downldSt.downloadPath = localPath;
    saveStateToFile();
    // signal downloadReady
    emit downloadReady(localPath);
    // schedule verify
    QTimer::singleShot(0, this, &FirmwareUpdateScheduler::onCheckDownloadedFile);
}

void FirmwareUpdateScheduler::onCheckDownloadedFile()
{
    // Run the python script 'evmega_firmware_update.py'

    // Check whether the result file exists
    try {
        if (fwupdateProc.state() != QProcess::ProcessState::NotRunning)
            throw QString("FirmwareUpdateScheduler: Process is still starting/running");
        // Prepare process to 'start'
        QStringList args;
        args << "prepare" << "-m" << charger::Platform::instance().firmwareInstallReadyFilePath
             << downldSt.downloadPath;

        qCDebug(FirmwareUpdate) << args;
        fwupdateProc.setProgram(charger::Platform::instance().firmwareUpdateScriptFilePath);
        fwupdateProc.setArguments(args);
        fwupdateProc.start(QIODevice::ReadOnly);
    }
    catch (QString &errStr)
    {
        qCWarning(FirmwareUpdate) << errStr;
    }
}

void FirmwareUpdateScheduler::onProcError(QProcess::ProcessError error)
{
    QString errStr;
    switch (error) {
    case QProcess::ProcessError::FailedToStart:
        errStr = QString("FirmwareUpdateScheduler error: %1 failed to start.").arg(fwupdateProc.program());
        qCWarning(FirmwareUpdate) << errStr;
        emit installFailed(errStr); // do not expect finish() signal
        break;
    case QProcess::ProcessError::Crashed:
        errStr = QString("FirmwareUpdateScheduler error: %1 crashed.").arg(fwupdateProc.program());
        qCWarning(FirmwareUpdate) << errStr;
        break;
    case QProcess::ProcessError::UnknownError:
        errStr = QString("FirmwareUpdateScheduler error: %1 has unknown error.").arg(fwupdateProc.program());
        qCWarning(FirmwareUpdate) << errStr;
    default:
        // Has the program died yet? Wait for the 'finish' callback?
        break;
    }
}

void FirmwareUpdateScheduler::onProcFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString errStr;
    switch (exitStatus) {
    case QProcess::ExitStatus::NormalExit:
        // exitCode cannot be relied upon (at least on Windows). // Check the downloaded file instead
        if (QFile::exists(charger::Platform::instance().firmwareInstallReadyFilePath)) {
            emit installReady(downldSt.downloadPath);
        }
        else {
            errStr = QString("FirmwareUpdateScheduler install-package finished: install-ready file not exist, assumed failed. Exit-code(%1); %2").arg(exitCode)
                    .arg(fwupdateProc.errorString());
            emit installFailed(errStr);
        }
        break;
    case QProcess::ExitStatus::CrashExit:
        errStr = QString("FirmwareUpdateScheduler install-package finished: %1 crashed.").arg(fwupdateProc.program());
        emit installFailed(errStr);
        break;
    }
}

void FirmwareUpdateScheduler::onProcStdoutReady()
{
    QByteArray stdoutBuf = fwupdateProc.readAllStandardOutput();
    qCWarning(FirmwareUpdate) << stdoutBuf.toStdString().c_str();
}

void FirmwareUpdateScheduler::onProcStderrReady()
{
    QByteArray stderrBuf = fwupdateProc.readAllStandardError();
    qCCritical(FirmwareUpdate) << stderrBuf.toStdString().c_str();
}

bool FirmwareUpdateScheduler::saveStateToFile()
{
    bool saveCompleted = false;

    QFile file(fuStateFilePath);
    try {
        if (! file.open(QIODevice::WriteOnly)) // implies Truncate
            throw std::runtime_error(file.errorString().toUtf8());
        QDataStream out(&file);

        // Write a header with a "magic number" and a version
        out << magicNumber;
        if (out.status() == QDataStream::Status::WriteFailed)
            throw std::runtime_error(file.errorString().toUtf8());
        out << verNumber;
        if (out.status() == QDataStream::Status::WriteFailed)
            throw std::runtime_error(file.errorString().toUtf8());
        out.setVersion(datastreamVer);

        // Write the data
        out << downldSt;
        if (out.status() == QDataStream::Status::WriteFailed)
            throw std::runtime_error(file.errorString().toUtf8());

        saveCompleted = true;
    }
    catch (std::exception &e) {
        qCWarning(FirmwareUpdate) << "FirmwareUpdateScheduler saveStateToFile error: " << e.what();
    }

    file.close();
    return saveCompleted;
}

bool FirmwareUpdateScheduler::loadStateFromFile()
{
    bool loaded = false;

    if (QFile::exists(fuStateFilePath)) {
        QFile file(fuStateFilePath);
        try {
            if (! file.open(QIODevice::ReadOnly))
                throw std::runtime_error(file.errorString().toUtf8());
            QDataStream in(&file);
            uint32_t magicNum;
            in >> magicNum;
            if (magicNum != this->magicNumber) {
                QString errStr = QString("Unexpected file magic-number (0x%1), abort.")
                        .arg(magicNum, 8, 16, QLatin1Char('0'));
                throw std::runtime_error(errStr.toUtf8());
            }
            uint32_t verNum;
            in >> verNum;
            if (verNum != this->verNumber) {
                QString errStr = QString("Unexpected file version-number (0x%1), abort.")
                        .arg(verNum, 8, 16, QLatin1Char('0'));
                throw std::runtime_error(errStr.toUtf8());
            }

            in.setVersion(datastreamVer);

            in >> downldSt;
            if (in.status() != QDataStream::Status::Ok) {
                QString errStr = QString("Read QHash error status=%1").arg(in.status());
                throw std::runtime_error(errStr.toUtf8());
            }
            loaded = true;
        }
        catch (std::exception &e) {
            qCWarning(FirmwareUpdate) << "FirmwareUpdateScheduler loadStateFromFile error: " << e.what();
        }
    }
    return loaded;
}
