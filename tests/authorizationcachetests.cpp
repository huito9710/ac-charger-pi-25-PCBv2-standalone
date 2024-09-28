#include "authorizationcachetests.h"
#include "authorizationcache.h"
#include "OCPP/ocppTypes.h"
#include <random>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

static std::vector<QString> randomTagIds(const int cnt, std::uniform_int_distribution<> &uidRand);
static std::vector<ocpp::IdTagInfo> randomTagInfo(const int cnt, std::uniform_int_distribution<> &authRand,
                                                  std::uniform_int_distribution<> &expDayOffsetRand);

AuthorizationCacheTests::AuthorizationCacheTests()
{
    using namespace ocpp;

    qDebug();
    qDebug();
    qDebug() << "****  Starting Authorization Cache ....";
    AuthorizationCache &acache = AuthorizationCache::getInstance();

    // Print loaded Entry from the beginning
    qDebug();
    qDebug();
    qDebug() << "****  Print 20 Items....";
    acache.printEntries();
    qDebug() << "*************************";

    qDebug() << " Bye ";
    return;

    std::uniform_int_distribution<> hexRand(0, 15);
    std::uniform_int_distribution<> authStatusRand(
                static_cast<int>(ocpp::AuthorizationStatus::_First),
                static_cast<int>(ocpp::AuthorizationStatus::_Last));
    std::uniform_int_distribution<> minutesAheadRand(1, 5);

    std::vector<QString> tagIds;
    std::vector<IdTagInfo> tagInfos;

    // Produce 20 tag infos with
    // - Random TagIds
    // - Random Status (Accepted vs Invalid)
    // - Random Expiry Date (1-5 minutes) if status is Valid
    tagIds = randomTagIds(20, hexRand);
    tagInfos = randomTagInfo(20, authStatusRand, minutesAheadRand);


    // Add Entry - 20 of them
    qDebug();
    qDebug() << "****  Add 20 Items....";
    for (int i = 0; i < 20; i++)
        acache.addEntry(tagIds[i], tagInfos[i]);
    qDebug();
    qDebug();
    qDebug() << "****  Print 20 Items....";
    acache.printEntries();

    // Check Entry -
    qDebug();
    qDebug();
    qDebug() << "****  Check 20 Items....";
    for (int i = 0; i < 20; i++) {
        bool valid = acache.checkValid(tagIds[i]);
        qDebug() << tagIds[i] << ": " << (valid? "Valid" : "Invalid");
    }
    qDebug();
    qDebug() << "****  Saving Cache to Disk....";
    acache.flushToDisk();
    qDebug() << "****  Done bye!....";
    return;

    // Update Entry - Update Status of first 10 of them
    qDebug();
    qDebug();
    qDebug() << "****  Update first 10 Items....";
    std::random_device rd;
    for (int i = 0; i < 10; i++) {
        AuthorizationStatus oldStatus = tagInfos[i].authStatus;
        tagInfos[i].authStatus = static_cast<ocpp::AuthorizationStatus>(authStatusRand(rd));
        acache.updateEntry(tagIds[i], tagInfos[i]);
        std::stringstream ss;
        ss << oldStatus << " -> " << tagInfos[i].authStatus;
        qDebug() << tagIds[i] << ": " << ss.str().c_str();
    }

    // Check Entry -
    qDebug();
    qDebug();
    qDebug() << "****  Check 20 Items....";
    for (int i = 0; i < 20; i++) {
        bool valid = acache.checkValid(tagIds[i]);
        qDebug() << tagIds[i] << ": " << (valid? "Valid" : "Invalid");
    }

    // Wait 2 minutes for expiry?
    qDebug();
    qDebug();
    qDebug() << "Make last 10 items Accepted....";
    for (int i = 10; i < 20; i++) {
        AuthorizationStatus oldStatus = tagInfos[i].authStatus;
        tagInfos[i].authStatus = AuthorizationStatus::Accepted;
        acache.updateEntry(tagIds[i], tagInfos[i]);
        std::stringstream ss;
        ss << oldStatus << " -> " << tagInfos[i].authStatus;
        qDebug() << tagIds[i] << ": " << ss.str().c_str();
    }

    // Check Entry - look for Expiry
    qDebug();
    qDebug();
    qDebug() << "Wait for 3 minutes....";
    QEventLoop loop;
    QTimer::singleShot(180000, &loop, SLOT(quit()));
    loop.exec();
    qDebug();
    qDebug() << "****  Check 20 Items....";
    for (int i = 0; i < 20; i++) {
        bool valid = acache.checkValid(tagIds[i]);
        qDebug() << tagIds[i] << ": " << (valid? "Valid" : "Invalid");
    }

    // RemoveIfExpired
    qDebug();
    qDebug() << "****  Removing Expired Items....";
    acache.removeNonValids();
    qDebug();
    qDebug() << "****  Check 20 Items....";
    for (int i = 0; i < 20; i++) {
        bool valid = acache.checkValid(tagIds[i]);
        qDebug() << tagIds[i] << ": " << (valid? "Valid" : "Invalid");
    }

    // RemoveIfOlderThan

    // Check Entry

    // RemoveAll
    qDebug();
    qDebug();
    qDebug() << "****  Removing All Items....";
    acache.removeAll();
    qDebug();
    qDebug() << "****  Check 20 Items....";
    for (int i = 0; i < 20; i++) {
        bool valid = acache.checkValid(tagIds[i]);
        qDebug() << tagIds[i] << ": " << (valid? "Valid" : "Invalid");
    }

}


static std::vector<QString> randomTagIds(const int cnt, std::uniform_int_distribution<> &uidRand)
{
    std::vector<QString> tagIds;
    std::random_device rd;
    for(int i = 0; i < cnt; i++) {
        std::ostringstream oss;
        for (int i = 0; i < 14; i++) {
            int hexDigit = uidRand(rd);
            oss << std::hex << hexDigit;
        }
        tagIds.push_back(QString(oss.str().c_str()));
    }
    return tagIds;
}

static std::vector<ocpp::IdTagInfo> randomTagInfo(const int cnt, std::uniform_int_distribution<> &authRand,
                                                  std::uniform_int_distribution<> &expDayOffsetRand)
{
    using namespace ocpp;
    std::random_device rd;
    std::vector<IdTagInfo> tagInfos;

     for(int i = 0; i < cnt; i++) {
         struct IdTagInfo tagInfo;
         tagInfo.authStatus = static_cast<ocpp::AuthorizationStatus>(authRand(rd));
         int minutesAhead = expDayOffsetRand(rd);
         ocpp::DateTime expDateUtc = std::chrono::system_clock::now();
         expDateUtc += std::chrono::minutes(minutesAhead);
         tagInfo.dateTime = expDateUtc;
         tagInfos.push_back(tagInfo);
     }

    return tagInfos;
}
