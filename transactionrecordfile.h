#ifndef TRANSACTIONRECORDFILE_H
#define TRANSACTIONRECORDFILE_H

#include <QString>
#include "chargerconfig.h"
#include "T_Type.h"

class TransactionRecordFile
{
public:
    TransactionRecordFile(const QString chargerNum, const charger::AcMeterPhase numPhase);
    bool addEntry(const QString transactionId, const QString tagId,
                  const EvChargingDetailMeterVal &startVal, const EvChargingDetailMeterVal &endVal);

private:
    const QString chrgRecCsvPath;
    const int maxNumRecords;
    const int minNumRecordsToKeep; // number of records to keep if max is reached
    const QString chargerNum;
    const charger::AcMeterPhase meterPhase;
};

#endif // TRANSACTIONRECORDFILE_H
