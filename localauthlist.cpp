#include "localauthlist.h"
#include <QDebug>

ocpp::LocalAuthorizationListVersion LocalAuthList:: getVersion() const
{
   std::unique_ptr<LocalAuthListDb::ListVersionInfo> listVer = listDb.getListVersion();
   if (listVer)
       return listVer->verNum;
   else
       throw std::runtime_error("ListVersion not available");
}

std::unique_ptr<ocpp::AuthorizationStatus> LocalAuthList::getAuthStatusById(std::string tag)
{
    // Convert Read-Tag to OCPP TagId
    ocpp::IdToken tagId;
    if (tag.length() > tagId.size())
        throw std::runtime_error("Input Tag UID too large. Not supported.");
    tagId.fill(0); // binary '0', see the AuthorizationData()
    std::copy(tag.begin(), tag.end(), tagId.begin());

    // Look up from database
    return listDb.lookupAuthStatus(tagId);
}

std::unique_ptr<ocpp::DateTime> LocalAuthList::getAuthExpireDateById(std::string tag)
{
    // Convert Read-Tag to OCPP TagId
    ocpp::IdToken tagId;
    if (tag.length() > tagId.size())
        throw std::runtime_error("Input Tag UID too large. Not supported.");
    tagId.fill('0'); // hexadecimal '0'
    std::copy(tag.begin(), tag.end(), tagId.begin());

    // Look up from database
    return listDb.lookupExpDate(tagId);
}


bool LocalAuthList::replaceAll(const std::vector<ocpp::AuthorizationData> &newTags)
{
    bool succ = false;
    Q_ASSERT(newTags.size() > 0);

    listDb.beginTransaction();
    try {
        // remove every records from database table
        if (listDb.deleteAll()) {
            // inserting these new Tags one by one
            std::vector<ocpp::AuthorizationData>::const_iterator itr = newTags.begin();
            while (itr != newTags.end()) {
                if (itr->idTagInfo) {
                    qDebug() << "Adding Tag " << ocpp::idTokenToStdString(itr->idTag).c_str();
                    if (!listDb.addOrUpdateOne(itr->idTag, itr->idTagInfo->authStatus, itr->idTagInfo->dateTime)) {
                        throw std::runtime_error("Add tag failed.");
                    }
                }
                else {
                    qWarning() << "LocalAuthList::replaceAll() skip tag "
                               << ocpp::idTokenToStdString(itr->idTag).c_str() << " because it is missing idTagInfo.";
                }
                itr++;
            }
            // ALL OK - commit transaction
            listDb.endTransaction(true);
            succ = true;
        }
        else {
            throw std::runtime_error("deleteAll() fails.");
        }
    }
    catch (std::exception &e){
        qWarning() << "LocalAuthList::replaceAll() " << e.what();
        listDb.endTransaction(false);
    }

    return succ;
}

bool LocalAuthList::update(const std::vector<ocpp::AuthorizationData>& tagsUpdate)
{
    bool succ = false;

    if (tagsUpdate.empty())
        return true; // There is nothing update; mark as completed

    listDb.beginTransaction();
    try {
        // inserting these new Tags one by one
        std::vector<ocpp::AuthorizationData>::const_iterator itr = tagsUpdate.begin();
        while (itr != tagsUpdate.end()) {
            if (itr->idTagInfo) {
                qDebug() << "Updating Tag " << ocpp::idTokenToStdString(itr->idTag).c_str();
                if (!listDb.addOrUpdateOne(itr->idTag, itr->idTagInfo->authStatus, itr->idTagInfo->dateTime))
                    throw std::runtime_error("Add/Update tag failed.");
            }
            else {
                qDebug() << "Deleting Tag " << ocpp::idTokenToStdString(itr->idTag).c_str();
                if (!listDb.deleteOne(itr->idTag))
                    throw std::runtime_error("Delete tag failed.");
            }
            itr++;
        }
        // ALL OK - commit transaction
        listDb.endTransaction(true);
        succ = true;
    }
    catch (std::exception &e){
        qWarning() << "LocalAuthList::update() " << e.what();
        listDb.endTransaction(false);
    }

    return succ;
}
