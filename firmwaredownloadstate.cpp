#include "firmwaredownloadstate.h"

FirmwareDownloadState::FirmwareDownloadState() :
    downloaded(true),
    availDateTime(QDateTime::currentDateTime()),
    retries(defaultNumDownloadRetries),
    retryInterval(defaultDownloadRetryInterval)
{
    remainAttempts = retries;
}

QDataStream &operator<<(QDataStream &ds, const FirmwareDownloadState &s)
{
    ds << s.downloaded;
    ds << s.downloadUri;
    ds << s.downloadPath;
    ds << s.remainAttempts;
    ds << s.availDateTime;
    ds << s.retries;
    long long intervalVal = s.retryInterval.count();
    ds << intervalVal;
    return ds;
}

QDataStream &operator>>(QDataStream &ds, FirmwareDownloadState &s)
{
    ds >> s.downloaded;
    ds >> s.downloadUri;
    ds >> s.downloadPath;
    ds >> s.remainAttempts;
    ds >> s.availDateTime;
    ds >> s.retries;
    long long intervalVal;
    ds >> intervalVal;
    s.retryInterval = std::chrono::seconds(intervalVal); // check portability on Pi build

    return ds;
}
