#ifndef FIRMWAREDOWNLOADSTATE_H
#define FIRMWAREDOWNLOADSTATE_H
#include <QDateTime>
#include <QDataStream>
#include <chrono>

struct FirmwareDownloadState
{
public:
    FirmwareDownloadState();
    bool downloaded;
    QString downloadUri;
    QString downloadPath;
    QDateTime availDateTime;
    int remainAttempts;
    int retries;
    std::chrono::seconds retryInterval;
    static constexpr int defaultNumDownloadRetries{2};
    static constexpr std::chrono::seconds defaultDownloadRetryInterval{120};
};

QDataStream &operator<<(QDataStream &ds, const FirmwareDownloadState &s);
QDataStream &operator>>(QDataStream &ds, FirmwareDownloadState &s);

#endif // FIRMWAREDOWNLOADSTATE_H
