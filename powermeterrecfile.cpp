#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "powermeterrecfile.h"
#include "hostplatform.h"

PowerMeterRecFile::PowerMeterRecFile(const QString chargerNum, const int numPhase)
    : chargerNum(chargerNum),
      meterPhase(numPhase)
{}

bool PowerMeterRecFile::addEntry(QDateTime dateTime, const EvChargingDetailMeterVal &meterVal)
{
    bool written = false;
    QString recCsvPath = QString("%1PowerMeterRecord-%2.csv")
            .arg(charger::Platform::instance().recordsDir)
            .arg(dateTime.toLocalTime().toString("yyyy-MM-dd"));

    QFile file(recCsvPath);
    if(file.open(QIODevice::Append|QIODevice::ReadWrite|QIODevice::Text))
    {
        QTextStream record(&file);
        if(file.size() == 0)
        {
            QString header = QString("Charger-Num,Date-Time,Energy,Power,Power-Factor");
            if (meterPhase > 1) {
                for (int i = 0; i < meterPhase; i++)
                    header.append(QString(",V%1").arg(i+1));
                for (int i = 0; i < meterPhase; i++)
                    header.append(QString(",I%1").arg(i+1));
            }
            else {
                header.append(",V,I");
            }
            header.append(",Freq");
            header.append("\n");
            record << header;
        }

        QString rowStr = QString("%1,%2,%3,%4,%5")
                .arg(chargerNum)
                .arg(dateTime.toLocalTime().toString(Qt::DateFormat::ISODate))
                .arg(meterVal.Q/10.0, 0, 'f', 1)
                .arg(meterVal.P/100.0, 0, 'f', 2)
                .arg(meterVal.Pf/1000.0, 0, 'f', 3);
        for (int i = 0; i < meterPhase; i++)
            rowStr.append(QString(",%1").arg(meterVal.V[i] / 100.0, 0, 'f', 2));
        for (int i = 0; i < meterPhase; i++)
            rowStr.append(QString(",%1").arg(meterVal.I[i] / 1000.0, 0, 'f', 3));
        rowStr.append(QString(",%1").arg(meterVal.Freq/100.0, 0, 'f', 2));
        rowStr.append("\n");
        record << rowStr;
        if (file.error()) {
            qWarning() << "Error writing Meter Records to " << recCsvPath << ": " << file.errorString();
            written = false;
        }
        else {
            written = true;
        }
        file.close();
    }
    return written;
}
