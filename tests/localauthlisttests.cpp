#include "localauthlisttests.h"

#include "localauthlist.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <sstream>
#include <memory>
#include <chrono>
#include <iostream>
#include <iomanip>
#include "OCPP/rpcmessage.h"
#include "OCPP/callmessage.h"

bool LocalAuthListTests::loadCallMsg(QString filename)
{
    // Parse Long List of Items received from OCPP-CS
    QFile msgFile(filename);
    if (msgFile.open(QFile::ReadOnly)) {
        QTextStream stream(&msgFile);
        QString data = stream.readAll();
        std::shared_ptr<ocpp::RpcMessage> rpcMsg = ocpp::CallMessage::fromMessageString(data.toStdString());
        auto callMsg = std::dynamic_pointer_cast<ocpp::CallMessage>(rpcMsg);
        QString id = QString::fromStdString(callMsg->getId());
        QString action = QString::fromStdString(callMsg->getAction());
        QString payload = QString::fromStdString(callMsg->getPayload());
        qDebug() << id << ":" << action <<  ": "
                 << ((payload.length() > 1000)? payload.left(1000) + "..." : payload);
        return true;
    }
    return false;
}

LocalAuthListTests::LocalAuthListTests()
{
    // Create Meta Data
    std::uniform_int_distribution<> hexRand(0, 15);
    std::uniform_int_distribution<> authStatusRand(
                static_cast<int>(ocpp::AuthorizationStatus::_First),
                static_cast<int>(ocpp::AuthorizationStatus::_Last));


    // Create Database
    LocalAuthList &authList = LocalAuthList::getInstance();

    try {
        qDebug() << "List Version: " << authList.getVersion();
    }
    catch (std::exception &e) {
        qWarning() << e.what();
    }

    // Full Update - 1
    std::vector<ocpp::AuthorizationData> dataset1 = randomData(101, hexRand, authStatusRand);
    if (authList.replaceAll(dataset1)) {
        authList.setVersion(10);
        qDebug() << "Full Update 1 completed.";
    }
    else {
        qFatal("Full Update 1 failed!");
    }



    // Full Update - 2
    qDebug() << "List Version: " << authList.getVersion();
    std::vector<ocpp::AuthorizationData> dataset2 = randomData(28, hexRand, authStatusRand);
    if (authList.replaceAll(dataset2)) {
        authList.setVersion(20);
        qDebug() << "Full Update 2 completed.";
    }
    else {
        qFatal("Full Update 2 failed!");
    }


    // Differential Update
    qDebug() << "List Version: " << authList.getVersion();

    // change the status and expiry-dates on a subset of the dataset2
    std::vector<ocpp::AuthorizationData> subsetDataset2(dataset2.begin() + 5, dataset2.begin() + 15);
    randomStatus(subsetDataset2, authStatusRand, hexRand);
    if (authList.update(subsetDataset2)) {
        authList.setVersion(21);
        qDebug() << "Diff Update to 21 completed.";
    }
    else {
        qFatal("Diff Update to 21 failed!");
    }

    qDebug() << "List Version: " << authList.getVersion();

    qDebug() << "Query Auth status and expiry date of each record...";

    // Try tag not in list
    std::unique_ptr<ocpp::AuthorizationStatus> authStatus = authList.getAuthStatusById("1234567890ABCDEF1234");
    if (authStatus)
        qWarning() << "Fail: Auth Status unexpectedly available for fakeTag!";
    else {
        qDebug() << "OK: auth-status missing for fake Tag as expected\n";
        // Try each tag in dataset2
        for (std::vector<ocpp::AuthorizationData>::const_iterator itr = dataset2.begin(); itr != dataset2.end(); itr++){
            std::string tagIdStr = ocpp::idTokenToStdString(itr->idTag);
            std::unique_ptr<ocpp::AuthorizationStatus> authStatus = authList.getAuthStatusById(tagIdStr);
            if (authStatus) {
                std::ostringstream oss;
                oss << *authStatus;
                qDebug() << "Auth-Status for " << tagIdStr.c_str() << ": " << oss.str().c_str();
                switch (*authStatus) {
                case ocpp::AuthorizationStatus::Accepted:
                case ocpp::AuthorizationStatus::ConcurrentTx:
                {
                    std::unique_ptr<ocpp::DateTime> expDate = authList.getAuthExpireDateById(tagIdStr);
                    if (expDate) {
                        auto expDate_c = std::chrono::system_clock::to_time_t(*expDate);
                        std::ostringstream oss;
                        oss << "Expire-Date: " << std::put_time(std::localtime(&expDate_c), "%c");
                        qDebug() << oss.str().c_str();
                    }
                    else {
                        qWarning() << "Expiry Date retrieval is MISSING!!!!";
                    }
                }
                    break;
                default:
                    break; // There is no expiry date
                }
            }
            else {
                qWarning() << "Auth-Status for " << tagIdStr.c_str() << " is --- Not Available ---";
                break;
            }
        }
    }

}


std::vector<ocpp::AuthorizationData> LocalAuthListTests::randomData(int numRecs,
                                std::uniform_int_distribution<> &uidRand,
                                std::uniform_int_distribution<> &authRand)
{
    std::vector<ocpp::AuthorizationData> data;
    std::random_device rd;

    while (numRecs--) {
        ocpp::AuthorizationData d;
        // UID: 7-Byte/14-characters
        d.idTag.fill('0');
        for (int i = 0; i < 14; i++) {
            int hexDigit = uidRand(rd);
            std::ostringstream oss;
            oss << std::hex << hexDigit;
            d.idTag[i] = oss.str()[0];
        }

        // Status
        d.idTagInfo = std::make_unique<ocpp::IdTagInfo>();
        d.idTagInfo->authStatus = static_cast<ocpp::AuthorizationStatus>(authRand(rd));

        // Expiry Date if Status is Accepted/ConcurrentTx
        switch (d.idTagInfo->authStatus) {
        case ocpp::AuthorizationStatus::Accepted:
        case ocpp::AuthorizationStatus::ConcurrentTx:
        {
            // borrow the uidRand to randomize +days
            std::chrono::hours plusDays(24 * uidRand(rd));
            d.idTagInfo->dateTime = std::chrono::system_clock::now() + plusDays;
        }
            break;
        default:
            // Do nothing
            break;
        }

        data.push_back(std::move(d));
    }
    return data;
}

bool LocalAuthListTests::randomStatus(std::vector<ocpp::AuthorizationData> &recs,
                                std::uniform_int_distribution<> &authRand,
                                std::uniform_int_distribution<> &dateRand)
{
    using namespace std;

    bool succ = false;
    std::random_device rd;

    for (vector<ocpp::AuthorizationData>::iterator itr = recs.begin();
         itr != recs.end(); itr++) {

        // Status
        itr->idTagInfo = std::make_unique<ocpp::IdTagInfo>();
        itr->idTagInfo->authStatus = static_cast<ocpp::AuthorizationStatus>(authRand(rd));

        // Expiry Date if Status is Accepted/ConcurrentTx
        switch (itr->idTagInfo->authStatus) {
        case ocpp::AuthorizationStatus::Accepted:
        case ocpp::AuthorizationStatus::ConcurrentTx:
        {
                std::chrono::hours plusDays(24 * dateRand(rd));
                itr->idTagInfo->dateTime = chrono::system_clock::now() + plusDays;
        }
            break;
        default:
            // Do nothing
            break;
        }

    }

    return succ;
}
