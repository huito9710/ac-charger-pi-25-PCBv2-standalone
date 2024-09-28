#include "authorizationcache.h"
#include <QDebug>
#include <QFile>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "hostplatform.h"
#include "logconfig.h"

AuthorizationCache::AuthorizationCache() :
    cacheFilePath(QString("%1authorization.cache").arg(charger::Platform::instance().authCacheDir)),
    maxCacheEntries(defaultMaxCacheEntries)
{
    loadFromFile();
    printEntries();
};

void AuthorizationCache::setMaxCacheEntries(int max)
{
    maxCacheEntries = max;
    if (cache.size() >= maxCacheEntries)
        freeUpSpace();
}

bool AuthorizationCache::addEntry(const QString &id, const ocpp::IdTagInfo &tagInfo)
{
    bool added = false;
    try {
        if(id.length() <= 0)
            throw std::invalid_argument("id is required.");
        CacheEntry entry(tagInfo);
        if (cache.size() >= maxCacheEntries)
            freeUpSpace();
        if (cache.insert(id, tagInfo) == cache.end())
            throw std::runtime_error("insert() returns failure.");
        added = true;
    }
    catch (std::invalid_argument &ia) {
        qCWarning(AuthCache) << "AuthorizationCache addEntry() argument error: " << ia.what();
        added = false;
    }
    catch (std::runtime_error &re) {
        qCWarning(AuthCache) << "AuthorizationCache addEntry failed: " << re.what();
        added = false;
    }

    return added;
}

bool AuthorizationCache::checkValid(const QString &id)
{
    bool isValid = false;

    try {
        QHash<QString, CacheEntry>::const_iterator ci = cache.constFind(id);
        if (ci == cache.end())
            throw std::runtime_error("find() returns empty");
        switch (ci->tag.authStatus) {
        case ocpp::AuthorizationStatus::Accepted:
        case ocpp::AuthorizationStatus::ConcurrentTx:
        {
            // check Expiry Date
            ocpp::DateTime utcCurr = std::chrono::system_clock::now();
            if ((!ci->tag.isDateTimeUnset()) && (utcCurr > ci->tag.dateTime)) {
                isValid = false;
                QHash<QString, CacheEntry>::iterator i = cache.find(id);
                // Mark as Expired
                i->tag.authStatus = ocpp::AuthorizationStatus::Expired;
            }
            else {
                isValid = true;
            }
        }
            break;
        default:
            isValid = false;
            break;
        }
    }
    catch (std::runtime_error &re) {
        qCWarning(AuthCache) << "AuthorizationCache checkValid failed: " << re.what();
        isValid = false;
    }

    return isValid;
}

bool AuthorizationCache::removeAll()
{
    bool removed = false;
    try {
        cache.clear();
        if (cache.size() == 0)
            removed = true;
        else
            throw std::runtime_error("size() returns non-zero.");
    }
    catch (std::runtime_error &re) {
        qCWarning(AuthCache) << "AuthorizationCache removeAll failed: " << re.what();
        removed = false;
    }

    return removed;
}

int AuthorizationCache::removeNonValids()
{
    int numRemoved = 0;

    int beforeCleanup = cache.size();
    qCDebug(AuthCache) << "Removing Non-Valids, starting with " << beforeCleanup << " entries.";
    QHash<QString, CacheEntry>::iterator i = cache.begin();
    while (i != cache.end()) {
        switch (i->tag.authStatus) {
        case ocpp::AuthorizationStatus::Expired:
        case ocpp::AuthorizationStatus::Blocked:
        case ocpp::AuthorizationStatus::Invalid:
            i = cache.erase(i); // returns the item after deleted item
            numRemoved++;
            break;
        default:
            i++;
            break;
        }
    }
    int afterCleanup = cache.size();
    qCDebug(AuthCache) << "Entries reduced by " << numRemoved << " to " << afterCleanup;

    return numRemoved;
}


int AuthorizationCache::removeIfOlderThanDays(const int days)
{
    int numRemoved = 0;

    int beforeCleanup = cache.size();
    qCDebug(AuthCache) << "Removing entries older than " << days << " days. Starting from "
                       << beforeCleanup << " entries.";
    QDateTime staleDate = QDateTime::currentDateTime().addDays(0 - days);
    QHash<QString, CacheEntry>::iterator i = cache.begin();
    while (i != cache.end()) {
        if (i->lastUpdated < staleDate) {
            i = cache.erase(i); // returns the item after deleted item
            numRemoved++;
        }
        else {
            i++;
        }
    }
    int afterCleanup = cache.size();
    qCDebug(AuthCache) << "Entries reduced by " << numRemoved << " to " << afterCleanup;

    return numRemoved;
}

int AuthorizationCache::removeIfOlderThanHours(const int hours)
{
    int numRemoved = 0;

    int beforeCleanup = cache.size();
    qCDebug(AuthCache) << "Removing entries older than " << hours << " hours. Starting from "
                       << beforeCleanup << " entries.";
    QDateTime staleDate = QDateTime::currentDateTime().addSecs(0 - (hours * 3600));
    QHash<QString, CacheEntry>::iterator i = cache.begin();
    while (i != cache.end()) {
        if (i->lastUpdated < staleDate) {
            i = cache.erase(i); // returns the item after deleted item
            numRemoved++;
        }
        else {
            i++;
        }
    }
    int afterCleanup = cache.size();
    qCDebug(AuthCache) << "Entries reduced by " << numRemoved << " to " << afterCleanup;
    return numRemoved;
}

int AuthorizationCache::removeOldest()
{
    int numRemoved = 0;

    int beforeCleanup = cache.size();
    qCDebug(AuthCache) << "Removing oldest entry. Starting from "
                       << beforeCleanup << " entries.";
    QHash<QString, CacheEntry>::iterator oldestEntry = cache.end();
    QHash<QString, CacheEntry>::iterator i = cache.begin();
    while (i != cache.end()) {
        if (oldestEntry == cache.end())
            oldestEntry = i;
        else if (i->lastUpdated < oldestEntry->lastUpdated) {
            oldestEntry = i;
        }
        i++;
    }
    if (oldestEntry != cache.end()) {
        cache.erase(oldestEntry); // returns the item after deleted item
        numRemoved++;
    }
    int afterCleanup = cache.size();
    qCDebug(AuthCache) << "Entries reduced by " << numRemoved << " to " << afterCleanup;
    return numRemoved;
}

bool AuthorizationCache::freeUpSpace()
{
    int freed = false;

    qCDebug(AuthCache) << "MaxCacheEntries: " << maxCacheEntries;
    qCDebug(AuthCache) << "Num Entries: " << cache.size();
    if (maxCacheEntries > cache.size()) {
        // enough space - no work need to be done
        freed = true;
    }
    else {
        // Remove Non-Valids first
        removeNonValids();
        if (maxCacheEntries > cache.size()) {
            freed = true;
        }
        else {
            // Try remove by expiry dates, keep recent
            std::array<int, 5> byDays = { 14, 7, 5, 3, 1 };
            for (int d : byDays) {
                removeIfOlderThanDays(d);
                if (maxCacheEntries > cache.size()) {
                    freed = true;
                    break;
                }
            }
            if (!freed) {
                std::array<int, 4> byHours = { 12, 6, 3, 1 };
                for (int h : byHours) {
                    removeIfOlderThanHours(h);
                    if (maxCacheEntries > cache.size()) {
                        freed = true;
                        break;
                    }
                }
            }
            // Try remove by oldest
           if (!freed) {
                freed = (removeOldest() > 0);
           }
        }
    }

    printEntries();
    return freed;
}

void AuthorizationCache::printEntries()
{
    QHash<QString, CacheEntry>::const_iterator ci = cache.cbegin();
    while (ci != cache.cend()) {
        std::stringstream ss;
        ss << "##### " << ci.key().toStdString() << " #####" << std::endl;
        ss << "Status: " << ci->tag.authStatus << std::endl;
        if (ci->tag.isDateTimeUnset()) {
            ss << "ExpiryDate: Not Set";
        }
        else {
            auto expDate_c = std::chrono::system_clock::to_time_t(ci->tag.dateTime);
            ss << "ExpiryDate: " << std::put_time(std::localtime(&expDate_c), "%c") << std::endl;
        }
        ss << "Added: " << ci->lastUpdated.toString(Qt::DateFormat::ISODate).toStdString() << std::endl;
        qCDebug(AuthCache) << ss.str().c_str();
        ci++;
    }
}

bool AuthorizationCache::flushToDisk() // save to disk
{
    return saveToFile();
}

bool AuthorizationCache::saveToFile()
{
    bool saveCompleted = false;

    QFile file(cacheFilePath);
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
        out << cache;
        if (out.status() == QDataStream::Status::WriteFailed)
            throw std::runtime_error(file.errorString().toUtf8());

        saveCompleted = true;
    }
    catch (std::exception &e) {
        qCWarning(AuthCache) << "AuthorizationCache saveToFile error: " << e.what();
    }

    file.close();
    return saveCompleted;
}

bool AuthorizationCache::loadFromFile()
{
    bool loaded = false;

    if (QFile::exists(cacheFilePath)) {
        QFile file(cacheFilePath);
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

            in >> cache;
            if (in.status() != QDataStream::Status::Ok) {
                QString errStr = QString("Read QHash error status=%1").arg(in.status());
                throw std::runtime_error(errStr.toUtf8());
            }
            loaded = true;
        }
        catch (std::exception &e) {
            qCWarning(AuthCache) << "AuthorizationCache loadFromFile error: " << e.what();
        }
    }
    return loaded;
}

QDataStream &operator<<(QDataStream &ds, const CacheEntry &ce)
{
    QDateTime expiryDate = (ce.tag.isDateTimeUnset())? QDateTime()
                : QDateTime::fromTime_t(std::chrono::system_clock::to_time_t(ce.tag.dateTime));
    ds << expiryDate;
    ds << QString(ocpp::idTokenToStdString(ce.tag.parentId).c_str());
    int authStatus = static_cast<int>(ce.tag.authStatus);
    ds << authStatus;
    ds << ce.lastUpdated;

    return ds;
}

QDataStream &operator>>(QDataStream &ds, CacheEntry &ce)
{

    QDateTime expiryDate;
    ds >> expiryDate;
    if (!expiryDate.isNull() && expiryDate.isValid())
        ce.tag.dateTime = std::chrono::system_clock::from_time_t(expiryDate.toTime_t());

    QString parentId;
    ds >> parentId;
    ce.tag.parentId.fill(0);
    if (!parentId.isNull() && !parentId.isEmpty())
        std::copy(parentId.toLocal8Bit().begin(), parentId.toLocal8Bit().end(), ce.tag.parentId.begin());
    int authStatus;
    ds >> authStatus;
    ce.tag.authStatus = static_cast<ocpp::AuthorizationStatus>(authStatus);
    ds >> ce.lastUpdated;

    return ds;
}
