#ifndef AUTHORIZATIONCACHE_H
#define AUTHORIZATIONCACHE_H

#include <QHash>
#include <QDateTime>
#include <QDataStream>
#include "OCPP/ocppTypes.h"

class CacheEntry {
public:
    struct ocpp::IdTagInfo tag;
    QDateTime lastUpdated;

    CacheEntry() = default;

    CacheEntry(const ocpp::IdTagInfo &tag_) {
        tag = tag_;
        lastUpdated = QDateTime::currentDateTime();

    }
};

QDataStream &operator<<(QDataStream &ds, const CacheEntry &c);
QDataStream &operator>>(QDataStream &ds, CacheEntry &c);

class AuthorizationCache
{
public:
    static AuthorizationCache& getInstance() {
        static AuthorizationCache theAuthCache;
        return theAuthCache;
    }
    void setMaxCacheEntries(int max);
    bool addEntry(const QString &id, const ocpp::IdTagInfo &tagInfo);
    bool checkValid(const QString &id);
    bool updateEntry(const QString &id, const ocpp::IdTagInfo &tagInfo) {
        return addEntry(id, tagInfo);
    }
    bool removeAll();
    int removeNonValids();
    int removeIfOlderThanDays(const int days);
    int removeIfOlderThanHours(const int hours);
    int removeOldest();
    void printEntries();
    bool flushToDisk(); // save to disk

private:
    static constexpr int defaultMaxCacheEntries = 1000;
    static constexpr uint32_t magicNumber = 0x24292164;
    ////////////////////////////////////////////////////////////////////////////////
    // Developer Note: must update both versions at the same time in the future
    static constexpr uint32_t verNumber = 0x512;
    static constexpr QDataStream::Version datastreamVer = QDataStream::Version::Qt_5_12;
    ////////////////////////////////////////////////////////////////////////////////
    QHash<QString, CacheEntry> cache;
    const QString cacheFilePath;
    int maxCacheEntries;

    AuthorizationCache();
    bool loadFromFile();
    bool saveToFile();
    bool freeUpSpace(); // free up space to have at least one slot within the maxCacheEntries
};

#endif // AUTHORIZATIONCACHE_H
