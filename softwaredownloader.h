#ifndef SOFTWAREDOWNLOADER_H
#define SOFTWAREDOWNLOADER_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class SoftwareDownloader : public QObject
{
    Q_OBJECT
public:
    SoftwareDownloader();
    bool Download(const QString &uri);
    bool StopDownload();
signals:
    void downloadCompleted(QString &path);
    void downloadFailed(QString &errStr);
private slots:
    void onProcTimerExpired();
    void onProcError(QProcess::ProcessError error);
    void onProcFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcStdoutReady();
    void onProcStderrReady();
private:
    const std::chrono::seconds timeout;
    const QString localDir;
    QProcess downloadProc;
    QTimer procTimer;
    QString localFilePath;
};

#endif // SOFTWAREDOWNLOADER_H
