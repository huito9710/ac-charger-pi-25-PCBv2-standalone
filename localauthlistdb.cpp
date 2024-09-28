#include <QDebug>
#include <QString>
#include <QSqlQuery>
#include <QSqlError>
#include <memory>

#include "localauthlistdb.h"

#include "hostplatform.h"

LocalAuthListDb::LocalAuthListDb()
    : dbFileName("ocpp_local_auth_list.db3"),
        schemaVer("1.0"),
        metaInfoTableName("list_info"),
        authStatusTableName("auth_status"),
        metaInfoFieldNames({
                            {SchemaVer, "SCHEMA_VER"},
                            {ListVersion, "LIST_VER"},
                            {ListUpdateDate, "LIST_LAST_UPDATE"}
                           }),
        authStatusFieldNames({
                            {TagId, "TAG_ID"},
                            {AuthStatus, "AUTH_STATUS"},
                            {ExpDate, "EXPIRY_DATE"}
                            }),
        db(QSqlDatabase::addDatabase("QSQLITE"))
{
    if (!db.isValid()) {
        qCritical() << "Cannot load SQLite Database Driver";
    }

    db.setDatabaseName(QString("%1%2").arg(charger::Platform::instance().localAuthListDir).arg(dbFileName.c_str()));
    if (db.open()) {
        // create table if not already there.
        createTables();
    }
    else {
        qCritical() << "Cannot open " << dbFileName.c_str();
    }
}


bool LocalAuthListDb::createTables()
{
    bool created = false;
    // create table if not already exists
    if (db.isOpen()) {
        QSqlQuery query(db);
        try {
            // Store 'DateTime' as INTEGER (Unix Time)
            QString createTblQryStr = QString("CREATE TABLE IF NOT EXISTS %1 ("
                                    "%2 TEXT NOT NULL,"
                                    "%3 INTEGER NOT NULL,"
                                    "%4 INTEGER NOT NULL"
                                    ");").arg(metaInfoTableName.c_str())
                                    .arg(metaInfoFieldNames.at(SchemaVer).c_str())
                                    .arg(metaInfoFieldNames.at(ListVersion).c_str())
                                    .arg(metaInfoFieldNames.at(ListUpdateDate).c_str());
            if (!query.exec(createTblQryStr))
                throw metaInfoTableName + ": " + query.lastError().text().toStdString();
            createTblQryStr = QString("CREATE TABLE IF NOT EXISTS %1 ("
                                      "%2 TEXT PRIMARY KEY UNIQUE NOT NULL COLLATE NOCASE,"
                                      "%3 INTEGER NOT NULL,"
                                      "%4 INTEGER"
                                      ");").arg(authStatusTableName.c_str())
                                      .arg(authStatusFieldNames.at(TagId).c_str())
                                      .arg(authStatusFieldNames.at(AuthStatus).c_str())
                                      .arg(authStatusFieldNames.at(ExpDate).c_str());
            if (!query.exec(createTblQryStr))
                throw authStatusTableName + ": " + query.lastError().text().toStdString();
            // Both tables created successfully
            created = true;
        }
        catch (std::string errStr)
        {
            qCritical() << "LocalAuthList Database create-table failed. " << errStr.c_str();
        }
    }
    else {
        qCritical() << "LocalAuthList Database connection is not opened to create tables.";
    }

    return created;
}

bool LocalAuthListDb::clearDataFromTable(const std::string tblName)
{
    bool cleared = false;

    // create table if not already exists
    if (db.isOpen()) {
        QSqlQuery query(db);
        try {
            // Store 'DateTime' as INTEGER (Unix Time)
            QString deleteTblQryStr = QString("DELETE FROM %1;").arg(tblName.c_str());
            if (!query.exec(deleteTblQryStr))
                throw std::runtime_error(tblName + ": " + query.lastError().text().toStdString());
            cleared = true;
        }
        catch (std::exception &e)
        {
            qCritical() << "LocalAuthList Database clear-table failed. " << e.what();
        }
    }
    else {
        qCritical() << "LocalAuthList Database connection is not opened to clear tables.";
    }

    return cleared;
}


//
// no-record: pointer is null
// database errors: exceptions (std::string) thrown
//
std::unique_ptr<LocalAuthListDb::ListVersionInfo> LocalAuthListDb::getListVersion() const
{
    std::unique_ptr<ListVersionInfo> verInfo;

    if (db.isOpen()) {
        QSqlQuery query(db);
        try {
            // Store 'DateTime' as INTEGER (Unix Time)
            QString selectQryStr = QString("SELECT %1, %3 FROM %2"
                                           " WHERE ROWID = (SELECT MAX(ROWID) FROM %2);")
                    .arg(metaInfoFieldNames.at(ListVersion).c_str())
                    .arg(metaInfoTableName.c_str())
                    .arg(metaInfoFieldNames.at(ListUpdateDate).c_str());
            if (!query.exec(selectQryStr))
                throw std::runtime_error(metaInfoTableName + ": " + query.lastError().text().toStdString());
            if (query.first() && query.isValid()) {
                verInfo = std::make_unique<ListVersionInfo>();
                // List Version
                QVariant verFieldVal = query.value(0);
                if (!verFieldVal.isValid() || verFieldVal.isNull()) {
                    throw std::runtime_error("List Version not available in database.");
                }
                else {
                    bool ok = false;
                    verInfo->verNum = verFieldVal.toInt(&ok);
                    if (!ok)
                        throw std::runtime_error("List Version is not an integer.");
                }
                // List Update Date
                QVariant lastUpdatedFieldVal = query.value(1);
                if (!lastUpdatedFieldVal.isValid() || lastUpdatedFieldVal.isNull()) {
                    throw std::runtime_error("List Update-Date not available in database.");
                }
                else {
                    bool ok = false;
                    std::chrono::seconds epoch {lastUpdatedFieldVal.toInt(&ok)};
                    if (!ok)
                        throw std::runtime_error("List Update-Date is not integer in database.");
                    verInfo->lastUpdate = ocpp::DateTime(epoch);
                }
            }
        }
        catch (std::exception &e)
        {
            qCritical() << "LocalAuthList Database query failed. " << e.what();
            throw; // re-throw to allow caller to handle query failures
        }
    }
    else {
        qCritical() << "LocalAuthList Database connection is not opened to retrieve LocalList version.";
    }

    return verInfo;
}

bool LocalAuthListDb::setVersion(ocpp::LocalAuthorizationListVersion listVer, ocpp::DateTime d)
{
    bool succ = false;

    if (db.isOpen()) {
        QSqlQuery query(db);
        try {
            // Store 'DateTime' as INTEGER (Unix Time)
            QString insertQryStr = QString("INSERT INTO %1 (%2, %3, %4)"
                                           " VALUES (:%2, :%3, :%4)")
                    .arg(metaInfoTableName.c_str())
                    .arg(metaInfoFieldNames.at(SchemaVer).c_str())
                    .arg(metaInfoFieldNames.at(ListVersion).c_str())
                    .arg(metaInfoFieldNames.at(ListUpdateDate).c_str());
            if (!query.prepare(insertQryStr))
                throw "Prepare-query list-version to database failed. " + query.lastError().text();
            query.bindValue(QString(":%1").arg(metaInfoFieldNames.at(SchemaVer).c_str()), this->schemaVer.c_str());
            query.bindValue(QString(":%1").arg(metaInfoFieldNames.at(ListVersion).c_str()), listVer);
            auto int_s = std::chrono::duration_cast<std::chrono::seconds, int64_t>(d.time_since_epoch());
            query.bindValue(QString(":%1").arg(metaInfoFieldNames.at(ListUpdateDate).c_str()), (unsigned long long)int_s.count());
            if (!query.exec())
                throw metaInfoTableName + ": " + query.lastError().text().toStdString();
            if (query.numRowsAffected() != 1)
                throw QString("Number of Rows Affected by Inserting List-Version is not 1 but %1").arg(query.numRowsAffected()).toStdString();
            succ = true;
        }
        catch (std::string errStr)
        {
            qCritical() << "LocalAuthList Database query failed. " << errStr.c_str();
            throw; // re-throw to allow caller to handle query failures
        }
    }
    else {
        qCritical() << "LocalAuthList Database connection is not opened to insert LocalList version.";
    }
    return succ;
}

std::unique_ptr<LocalAuthListDb::AuthStatusExpiryPair> LocalAuthListDb::lookupTagRecordById(const ocpp::IdToken &tagId)
{
    std::unique_ptr<LocalAuthListDb::AuthStatusExpiryPair> rec;

    if (db.isOpen()) {
        try {
            if (findOneTagQry.lastQuery().isEmpty()) {
                findOneTagQry = QSqlQuery(db);
                if (!findOneTagQry.prepare(QString("SELECT %1, %2 FROM %3"
                                              " WHERE %4 = :%4 COLLATE NOCASE")
                              .arg(authStatusFieldNames.at(AuthStatus).c_str())
                              .arg(authStatusFieldNames.at(ExpDate).c_str())
                              .arg(authStatusTableName.c_str())
                              .arg(authStatusFieldNames.at(TagId).c_str()))) {
                    std::ostringstream oss;
                    oss << "Prepare list-version to database failed. " << findOneTagQry.lastError().text().toStdString();
                    throw std::runtime_error(oss.str());
                }
            }

            findOneTagQry.bindValue(QString(":%1").arg(authStatusFieldNames.at(TagId).c_str()), QString::fromLatin1(tagId.data(), tagId.size()));
            if (!findOneTagQry.exec())
                throw authStatusTableName + ": " + findOneTagQry.lastError().text().toStdString();
            if (findOneTagQry.first() && findOneTagQry.isValid()) {
                // AuthStatus Field
                QVariant verFieldVal = findOneTagQry.value(0);
                if (!verFieldVal.isValid() || verFieldVal.isNull()) {
                    throw std::runtime_error("Authorization-Status field not available in database.");
                }
                else {
                    bool ok = false;
                    int verNum = verFieldVal.toInt(&ok);
                    if (ok) {
                        // convert to enum
                        bool enumConverted = false;
                        for (int i = static_cast<int>(ocpp::AuthorizationStatus::_First); i <= static_cast<int>(ocpp::AuthorizationStatus::_Last); i += 1) {
                            if (i == verNum) {
                                enumConverted = true;
                                rec = std::make_unique<AuthStatusExpiryPair>();
                                rec->first = static_cast<ocpp::AuthorizationStatus>(verNum);
                                break;
                            }
                        }
                        if (!enumConverted)
                            throw std::runtime_error(QString("Unexpected authorization-status value: %1").arg(verNum).toStdString());

                        // Expiry-Date field (integer)
                        verFieldVal = findOneTagQry.value(1);
                        if (verFieldVal.isValid() && !verFieldVal.isNull()) {
                            qlonglong expEpoch = verFieldVal.toLongLong(&ok);
                            if (ok) {
                                rec->second = std::chrono::time_point<ocpp::Clock>(std::chrono::seconds(expEpoch));
                            }
                            else {
                                throw std::runtime_error("Expiry-Date field is not an longlong.");
                            }
                        }
                        else {
                            throw std::runtime_error("Expiry-Date field not available.");
                        }
                    }
                    else {
                        throw std::runtime_error("Authorization-Status field is not an integer.");
                    }
                }
            }
        }
        catch (std::exception &e)
        {
            qCritical() << "LocalAuthList Database query failed. " << e.what();
            qDebug() << "Last Query: " << findOneTagQry.lastQuery();
            throw; // re-throw to allow caller to handle query failures
        }
    }
    else {
        qCritical() << "LocalAuthList Database connection is not opened to insert LocalList version.";
    }

    return rec;
}


//
// no-record: pointer is null
// database errors: exceptions (std::string) thrown
//
std::unique_ptr<ocpp::AuthorizationStatus> LocalAuthListDb::lookupAuthStatus(const ocpp::IdToken &tagId)
{
    std::unique_ptr<ocpp::AuthorizationStatus> authStatus;
    std::unique_ptr<AuthStatusExpiryPair> rec = lookupTagRecordById(tagId);
    if (rec)
        authStatus = std::make_unique<ocpp::AuthorizationStatus>(rec->first);
    return authStatus;
}

std::unique_ptr<ocpp::DateTime> LocalAuthListDb::lookupExpDate(const ocpp::IdToken &tagId)
{
    std::unique_ptr<ocpp::DateTime> expDate;
    std::unique_ptr<AuthStatusExpiryPair> rec = lookupTagRecordById(tagId);
    if (rec)
        expDate = std::make_unique<ocpp::DateTime>(rec->second);
    return expDate;
}


bool LocalAuthListDb::beginTransaction()
{
    bool succ = false;
    if (db.isOpen()) {
        succ = db.transaction();
    }
    else {
        qCritical() << "LocalAuthList Database connection is not opened to begin transaction.";
    }
    return succ;
}



// commit==false => rollback
bool LocalAuthListDb::endTransaction(bool commit) // true: commit, false: rollback
{
    bool succ = false;
    if (db.isOpen()) {
        if (commit)
            succ = db.commit();
        else
            succ = db.rollback();
    }
    else {
        qCritical() << "LocalAuthList Database connection is not opened to end transaction.";
    }
    return succ;
}

bool LocalAuthListDb::addOrUpdateOne(const ocpp::IdToken &tagId, ocpp::AuthorizationStatus authStatus)
{
    return replaceTagRecord(tagId, authStatus, nullptr);
}

bool LocalAuthListDb::addOrUpdateOne(const ocpp::IdToken &tagId, ocpp::AuthorizationStatus authStatus, const ocpp::DateTime &expireDate)
{

    return replaceTagRecord(tagId, authStatus, std::make_unique<ocpp::DateTime>(expireDate));
}

bool LocalAuthListDb::replaceTagRecord(const ocpp::IdToken &tagId, ocpp::AuthorizationStatus authStatus, std::unique_ptr<ocpp::DateTime> expireDate)
{
    bool succ = false;

    if (db.isOpen()) {
        try {
            if (replaceTagQry.lastQuery().isEmpty()) {
                replaceTagQry = QSqlQuery(db);
                if (!replaceTagQry.prepare(QString("REPLACE INTO %1 (%2, %3, %4)"
                                              " VALUES(:%2, :%3, :%4);")
                                .arg(authStatusTableName.c_str())
                                .arg(authStatusFieldNames.at(TagId).c_str())
                                .arg(authStatusFieldNames.at(AuthStatus).c_str())
                                .arg(authStatusFieldNames.at(ExpDate).c_str()))) {
                    throw std::runtime_error("Prepare add/update tag to database failed. " + replaceTagQry.lastError().text().toStdString());
                }
            }
            replaceTagQry.bindValue(QString(":%1").arg(authStatusFieldNames.at(TagId).c_str()), QString::fromLatin1(tagId.data(), tagId.size()));
            replaceTagQry.bindValue(QString(":%1").arg(authStatusFieldNames.at(AuthStatus).c_str()), static_cast<int>(authStatus));
            QVariant expireDateFieldVal = QVariant(); // null by default
            if (expireDate) // convert to seconds
                expireDateFieldVal = QVariant(std::chrono::duration_cast<std::chrono::seconds, int64_t>(expireDate->time_since_epoch()).count());
            replaceTagQry.bindValue(QString(":%1").arg(authStatusFieldNames.at(ExpDate).c_str()), expireDateFieldVal);
            if (replaceTagQry.exec()) {
                if (replaceTagQry.numRowsAffected() == 1)
                    succ = true;
                else
                    throw std::runtime_error(authStatusTableName + ": replace tag affects not equals to 1-row");
            }
            else {
                throw std::runtime_error(authStatusTableName + ": replace execution failed. " + replaceTagQry.lastError().text().toStdString());
            }
        }
        catch (std::exception &e) {
            qCritical() << "LocalAuthList Database query failed. " << e.what();
        }
    }
    else {
        qCritical() << "LocalAuthList Database connection is not opened to end transaction.";
    }

    return succ;
}

bool LocalAuthListDb::deleteOne(const ocpp::IdToken &tagId)
{
    bool succ = false;

    if (db.isOpen()) {
        try {
            if (removeOneTagQry.lastQuery().isEmpty()) {
                removeOneTagQry = QSqlQuery(db);
                if (!removeOneTagQry.prepare(QString("DELETE FROM %1 "
                                              "WHERE %2 = :%2);")
                                .arg(authStatusTableName.c_str())
                                .arg(authStatusFieldNames.at(TagId).c_str())
                                )) {
                    throw "Prepare add/update tag to database failed. " + removeOneTagQry.lastError().text();
                }
            }
            removeOneTagQry.bindValue(QString(":%1").arg(authStatusFieldNames.at(TagId).c_str()), QString::fromLatin1(tagId.data(), tagId.size()));
            if (removeOneTagQry.exec()) {
                if (removeOneTagQry.numRowsAffected() == 1)
                    succ = true;
                else
                    throw authStatusTableName + ": remove one tag affects not equals to 1-row";
            }
            else {
                throw authStatusTableName + ": delete execution failed. " + removeOneTagQry.lastError().text().toStdString();
            }
        }
        catch (std::string errStr) {
            qCritical() << "LocalAuthList Database query failed. " << errStr.c_str();
        }
    }
    else {
        qCritical() << "LocalAuthList Database connection is not opened to end transaction.";
    }

    return succ;

}
