#ifndef LOCALAUTHLIST_H
#define LOCALAUTHLIST_H

#include <chrono>
#include <vector>
#include "OCPP/ocppTypes.h"
#include "localauthlistdb.h"

// Singleton
class LocalAuthList
{
public:
    /*
     * Singleton Implementation: Follow C++11 suggestion here
     * https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
     */
    static LocalAuthList& getInstance() {
        static LocalAuthList theList;

        return theList;
    }
    // No copy and assignment
    LocalAuthList(const LocalAuthList&) = delete;
    void operator=(const LocalAuthList&) = delete;

    // Meta information
    ocpp::LocalAuthorizationListVersion getVersion() const; // throws if not available
    bool setVersion(ocpp::LocalAuthorizationListVersion listVer) {
        return listDb.setVersion(listVer); };

    // Lookup - return 'nullptr' if not found
    std::unique_ptr<ocpp::AuthorizationStatus> getAuthStatusById(std::string tag);
    std::unique_ptr<ocpp::DateTime> getAuthExpireDateById(std::string tag);

    // UpdateWithOcpp Objects
    bool removeAll() { return listDb.deleteAll(); };
    bool replaceAll(const std::vector<ocpp::AuthorizationData>&);
    bool update(const std::vector<ocpp::AuthorizationData>&);

private:
    LocalAuthList() {};
    LocalAuthListDb listDb;
};

#endif // LOCALAUTHLIST_H
