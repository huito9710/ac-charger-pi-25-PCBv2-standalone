#include <QFile>
#include "transactionrecordfile.h"
#include "hostplatform.h"
#include "api_function.h"
#include "ccharger.h"

TransactionRecordFile::TransactionRecordFile(const QString chargerNum, const charger::AcMeterPhase numPhase)
    : chrgRecCsvPath(QString("%1ChargeRecord.csv").arg(charger::Platform::instance().recordsDir)),
      maxNumRecords(100000),
      minNumRecordsToKeep(500),
      chargerNum(chargerNum),
      meterPhase(numPhase)
{}

bool TransactionRecordFile::addEntry(const QString transactionId, const QString tagId,
              const EvChargingDetailMeterVal &startVal, const EvChargingDetailMeterVal &endVal)
{
    bool written = false;

    QFile file(chrgRecCsvPath);
    if(file.size() > maxNumRecords)
        RemoveOldRecords(file, minNumRecordsToKeep);
    if(file.open(QIODevice::Append|QIODevice::ReadWrite|QIODevice::Text))
    {
        QTextStream record(&file);
        if(file.size() ==0)
        {
            record <<QObject::tr("TransactionID, Start Time, End Time, User, Duration, KWh, ChargerNo, Phase\n");
            //record <<QObject::trUtf8("TransactionID, 開始時間, 完成時間, 用戶名稱, 充電時長, 充電量, 充電樁編號, 相位\n");
        }

        QString localStartDtStr = startVal.meteredDt.toLocalTime().toString(Qt::DateFormat::ISODate);
        QString localStopDtStr = endVal.meteredDt.toString(Qt::DateFormat::ISODate);
        qint64 durationMSecs = startVal.meteredDt.msecsTo(endVal.meteredDt);
        QTime durationTime = QTime::fromMSecsSinceStartOfDay(durationMSecs);
        QString durationTimeStr = durationTime.toString("hh:mm");
        int numPhases = meterPhase == charger::AcMeterPhase::SPM93? 3 : 1;
        double chargedAmt = CCharger::meterValSubtract(startVal.Q, endVal.Q) / 10.0;
        record <<QString("%1, %2, %3, %4, %5, %6, %7, %8\n").arg(transactionId).arg(localStartDtStr)
                 .arg(localStopDtStr).arg(tagId).arg(durationTimeStr).arg(chargedAmt, 0, 'f', CCharger::MeterDecPts)
                 .arg(chargerNum).arg(numPhases);
        if (file.error()) {
            qWarning() << "Error writing Transaction Records to " << chrgRecCsvPath << ": " << file.errorString();
            written = false;
        }
        else {
            written = true;
        }
        file.close();
    }
    return written;
}
