#ifndef LOCALAUTHLISTTESTS_H
#define LOCALAUTHLISTTESTS_H

#include <vector>
#include <random>
#include "OCPP/ocppTypes.h"

class LocalAuthListTests
{
public:
    LocalAuthListTests();
    static bool loadCallMsg(QString filename);

private:
    std::vector<ocpp::AuthorizationData> randomData(int numRecs,
                                    std::uniform_int_distribution<> &uidRand,
                                    std::uniform_int_distribution<> &authRand);
    bool randomStatus(std::vector<ocpp::AuthorizationData> &recs,
                                    std::uniform_int_distribution<> &authRand,
                                    std::uniform_int_distribution<> &dateRand);
};

#endif // LOCALAUTHLISTTESTS_H
