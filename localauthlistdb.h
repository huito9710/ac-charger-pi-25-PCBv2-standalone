#ifndef LOCALAUTHLISTDB_H
#define LOCALAUTHLISTDB_H

#include <string>
#include <map>
#include <chrono>
#include <memory>
#include "OCPP/ocppTypes.h"
#include <QSqlDatabase>
#include <QSqlQuery>

class LocalAuthListDb
{
public:
    LocalAuthListDb();

    struct ListVersionInfo {
        ocpp::LocalAuthorizationListVersion verNum;
        ocpp::DateTime lastUpdate;
    };

    // meta-data
    std::string getSchemaVersion() const { return schemaVer; };
    std::unique_ptr<LocalAuthListDb::ListVersionInfo>  getListVersion() const;
    bool setVersion(ocpp::LocalAuthorizationListVersion listVer, ocpp::DateTime d = ocpp::Clock::now());

    // Read
    std::unique_ptr<ocpp::AuthorizationStatus> lookupAuthStatus(const ocpp::IdToken &tagId);
    std::unique_ptr<ocpp::DateTime> lookupExpDate(const ocpp::IdToken &tagId);

    // Transactional
    bool beginTransaction();
    bool endTransaction(bool commit); // true: commit, false: rollback

    // Create, Update, Delete
    bool addOrUpdateOne(const ocpp::IdToken &tagId, ocpp::AuthorizationStatus authStatus);
    bool addOrUpdateOne(const ocpp::IdToken &tagId, ocpp::AuthorizationStatus authStatus, const ocpp::DateTime &expireDate);
    bool deleteOne(const ocpp::IdToken &tagId);
    bool deleteAll() { return clearDataFromTable(authStatusTableName); }

private:
    bool createTables();
    bool clearDataFromTable(const std::string tblName);
    bool replaceTagRecord(const ocpp::IdToken &tagId, ocpp::AuthorizationStatus authStatus, std::unique_ptr<ocpp::DateTime> expireDate);

    using AuthStatusExpiryPair = std::pair<ocpp::AuthorizationStatus, ocpp::DateTime>;
    std::unique_ptr<AuthStatusExpiryPair> lookupTagRecordById(const ocpp::IdToken &tagId);

private:
    enum MetaInfoSchemaFields {
        SchemaVer,
        ListVersion,
        ListUpdateDate,
    };
    enum AuthStatusSchemaFields {
        TagId,
        AuthStatus,
        ExpDate,
    };
    const std::string dbFileName;
    const std::string schemaVer; // corresponds to how the tables are created in the code.
    const std::string metaInfoTableName;
    const std::string authStatusTableName;
    const std::map<MetaInfoSchemaFields, std::string> metaInfoFieldNames;
    const std::map<AuthStatusSchemaFields, std::string> authStatusFieldNames;

    QSqlDatabase db;
    QSqlQuery findOneTagQry; // prepared Query
    QSqlQuery replaceTagQry; // prepared Query
    QSqlQuery removeOneTagQry; // prepared Query
};

#endif // LOCALAUTHLISTDB_H
