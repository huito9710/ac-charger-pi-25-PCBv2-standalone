#ifndef POWERMETERRECFILE_H
#define POWERMETERRECFILE_H

#include <QString>
#include <QDateTime>
#include "chargerconfig.h"
#include "T_Type.h"

//
// Store the following record to a CSV file
// A new CSV file will be created every day.
// CSV record
// - Charger-Number
// - Date-Time
// - Energy
// - Power
// - Power-Factor
// - V1
// - V2 (only if phases > 1)
// - V3 (only if phases > 1)
// - I1
// - I2 (only if phases > 1)
// - I2 (only if phases > 1)
// - Frequency
//
class PowerMeterRecFile
{
public:
    PowerMeterRecFile(const QString chargerNum, const int numPhases);
    bool addEntry(QDateTime dateTime, const EvChargingDetailMeterVal &meterVal);

private:
    const QString chargerNum;
    const int meterPhase;
};

#endif // POWERMETERRECFILE_H
