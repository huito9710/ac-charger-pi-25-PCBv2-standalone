#include "evchargingtransaction.h"
#include "logconfig.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QSharedPointer<EvChargingTransaction> EvChargingTransaction::currentTransc = nullptr;
        //QSharedPointer<EvChargingTransaction>(new EvChargingTransaction(QDateTime::currentDateTime(),0,"-1"));

EvChargingTransaction::EvChargingTransaction(const QDateTime startDt, const ulong startMeterVal, const QString authorizedIdToken)
    :st(Status::Unknown),
      startDt(startDt),
      endDt(),
      startMeterVal(startMeterVal),
      endMeterVal(startMeterVal),
      stopReason(ocpp::Reason::Local),
      transactionId(-1),
      authId(authorizedIdToken),
      lastSentMeterValIdx(-1)
{
    if (authId.isNull() || authId.isEmpty())
        qCWarning(Transactions) << "EvChargingTransaction created with null or empty authId.";
}


void EvChargingTransaction::setTransactionId(const QString transId, int meterValIdx)
{
    if (transactionId >= 0) {
        qCWarning(Transactions) << "Over-writing transaction Id " << transactionId << " with " << transId;
    }
    bool converted = false;
    long newVal = transId.toLong(&converted);
    if (converted) {
        transactionId = newVal;
        // Only apply meterValIdx if transId is valid.
        lastSentMeterValIdx = meterValIdx;
    }
    else {
        qCWarning(Transactions) << "New transaction-Id is invalid format: " << transId;
    }
}

void EvChargingTransaction::setActive()
{
    switch (st) {
    case Status::Unknown:
        st = Status::Active;
        break;
    default:
        qCWarning(Transactions) << "Unexpected request to change EvChargingTransaction status from "
                                << static_cast<int>(st) << " to Active";
        break;
    }
}

 // No charging happened
void EvChargingTransaction::setCanceled(const QDateTime cancelDt, const ulong cancelMeterVal, const ocpp::Reason cancelReason)
{
    switch (st) {
    case Status::Active:
        st = Status::Canceled;
        this->endDt = cancelDt;
        this->endMeterVal = cancelMeterVal;
        this->stopReason = cancelReason;
        break;
    default:
        qCWarning(Transactions) << "Unexpected request to change EvChargingTransaction status from "
                                << static_cast<int>(st) << " to Canceled";
        break;
    }
}

void EvChargingTransaction::updateMeterVal(EvChargingDetailMeterVal meterVal)
{
    switch (st) {
    case Status::Active:
        meterValUpdates.append(meterVal);
        break;
    default:
        qCWarning(Transactions) << "Unexpected request to add Meter Value samples "
                                << "in Status " << static_cast<int>(st);
        break;
    }
}

EvChargingDetailMeterVal EvChargingTransaction::getMeterVal(int idx, bool *ok)
{
    bool inbound = false;
    if (idx >= 0 && idx < meterValUpdates.count()) {
        inbound = true;
    }
    if (ok != nullptr)
        *ok = inbound;

    return meterValUpdates.value(idx);
}

void EvChargingTransaction::setFinished(const QDateTime endDt, const ulong endMeterVal, const ocpp::Reason stopReason)
{
    switch (st) {
    case Status::Active:
        st = Status::Finished;
        this->endDt = endDt;
        this->endMeterVal = endMeterVal;
        this->stopReason = stopReason;
        break;
    default:
        qCWarning(Transactions) << "Unexpected request to change EvChargingTransaction status from "
                                << static_cast<int>(st) << " to Finished";
        break;
    }
}

void EvChargingTransaction::saveToFile(TransactionRecordFile &recFile)
{
    EvChargingDetailMeterVal startVal;
    EvChargingDetailMeterVal endVal;

    startVal.meteredDt = startDt;
    startVal.Q = startMeterVal;
    endVal.meteredDt = endDt;
    endVal.Q = endMeterVal;

    recFile.addEntry(transactionId > 0? QString::number(transactionId) : QString(),
                     authId, startVal, endVal);
}

QStringList EvChargingTransaction::toStringList()
{
    Q_ASSERT(st == Status::Finished);

    QStringList lines;
    if (st == Status::Finished) {
        QString line;

        line = QString("%1,%2,%3,%4,%5,%6")
                .arg(static_cast<int>(RecordLineType::TransStart))
                .arg(startDt.toString(Qt::DateFormat::ISODate))
                .arg(startMeterVal)
                .arg(authId)
                .arg(transactionId)
                .arg(lastSentMeterValIdx);
        lines.append(line);

        for (auto i = meterValUpdates.cbegin(); i != meterValUpdates.cend(); i++) {
            line = QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12")
                    .arg(static_cast<int>(RecordLineType::MeterValueUpdate))
                    .arg(i->meteredDt.toString(Qt::DateFormat::ISODate))
                    .arg(i->Q)
                    .arg(i->P)
                    .arg(i->I[0]).arg(i->I[1]).arg(i->I[2])
                    .arg(i->V[0]).arg(i->V[1]).arg(i->V[2])
                    .arg(i->Pf)
                    .arg(i->Freq);
            lines.append(line);
        }

        line = QString("%1,%2,%3,%4").arg(static_cast<int>(RecordLineType::TransStop))
                .arg(endDt.toString(Qt::DateFormat::ISODate))
                .arg(endMeterVal)
                .arg(static_cast<int>(stopReason));
        lines.append(line);
    }

    return lines;
}

QSharedPointer<EvChargingTransaction> EvChargingTransaction::fromStringList(const QStringList lines)
{
    QSharedPointer<EvChargingTransaction> trans;

    Q_ASSERT(lines.count() >= 2); // at least start and stop
    try {
        if (lines.count() < 2)
            throw std::runtime_error("Expecting at least 3 rows.");
        // expects the first one be Start
        QString line = lines.constFirst();
        QStringList fields = line.split(',');
        if (fields.count() < 6)
            throw std::runtime_error("Expected at least 6 fields for Start-Transaction record");
        if (fields[0].trimmed().toInt() != static_cast<int>(RecordLineType::TransStart))
            throw std::runtime_error("Unexpected field-type value: " + fields[0].toStdString());
        QDateTime startDt = QDateTime::fromString(fields[1], Qt::DateFormat::ISODate);
        if (startDt.isNull())
            throw std::runtime_error("Unexpected start-date field value: " + fields[1].toStdString());
        bool converted = false;
        unsigned long startMeterVal = fields[2].trimmed().toULong(&converted);
        if (!converted)
            throw std::runtime_error("Unexpected start-meter-value field value: " + fields[2].toStdString());
        QString authTagId  = fields[3];
        long transactionId = fields[4].trimmed().toLong(&converted);
        if (!converted)
            throw std::runtime_error("Unexpected transaction-Id field value: " + fields[4].toStdString());
        int lastSentMeterIdx = fields[5].trimmed().toInt(&converted);
        if (!converted)
            throw std::runtime_error("Unexpected last-sent-meter-value-idx field value: " + fields[5].toStdString());
        trans = QSharedPointer<EvChargingTransaction>::create(startDt, startMeterVal, authTagId);
        if (transactionId >= 0) {
            trans->setTransactionId(QString::number(transactionId), lastSentMeterIdx);
        }
        trans->setActive(); // this is needed to set the status correctly
        // Add meter value updates
        for (int i = 1; i < lines.count() - 1; i++) {
            fields = lines[i].split(',');
            if (fields.count() < 12)
                throw std::runtime_error("Expected at least 12 fields for Meter-Value records");
            if (fields[0].trimmed().toInt() != static_cast<int>(RecordLineType::MeterValueUpdate))
                throw std::runtime_error("Unexpected field-type value: " + fields[0].toStdString());
            EvChargingDetailMeterVal meterVal;
            meterVal.meteredDt = QDateTime::fromString(fields[1], Qt::DateFormat::ISODate);
            if (meterVal.meteredDt.isNull())
                throw std::runtime_error("Unexpected start-date field value: " + fields[1].toStdString());
            meterVal.Q = fields[2].trimmed().toULong(&converted);
            if (!converted)
                throw std::runtime_error("Unexpected meter-value Q field value: " + fields[2].toStdString());
            meterVal.P = fields[3].trimmed().toULong(&converted);
            if (!converted)
                throw std::runtime_error("Unexpected meter-value P field value: " + fields[3].toStdString());
            for (int i = 0, j = 4; i < 3; i++, j++) {
                meterVal.I[i] = fields[j].trimmed().toULong(&converted);
                if (!converted)
                    throw std::runtime_error("Unexpected meter-value I[" + std::to_string(i) + "] field value: " + fields[j].toStdString());
            }
            for (int i = 0, j = 7; i < 3; i++, j++) {
                meterVal.V[i] = fields[j].trimmed().toULong(&converted);
                if (!converted)
                    throw std::runtime_error("Unexpected meter-value V[" + std::to_string(i) + "] field value: " + fields[j].toStdString());
            }
            meterVal.Pf = fields[10].trimmed().toULong(&converted);
            if (!converted)
                throw std::runtime_error("Unexpected meter-value Pf field value: " + fields[10].toStdString());
            meterVal.Freq = fields[11].trimmed().toULong(&converted);
            if (!converted)
                throw std::runtime_error("Unexpected meter-value Freq field value: " + fields[11].toStdString());
            trans->updateMeterVal(meterVal);
        }
        // expects the last one be Stop
        line = lines.constLast();
        fields = line.split(',');
        if (fields.count() < 4)
            throw std::runtime_error("Expected at least 4 fields for Stop-Transaction record");
        if (fields[0].trimmed().toInt() != static_cast<int>(RecordLineType::TransStop))
            throw std::runtime_error("Unexpected field-type value: " + fields[0].toStdString());
        QDateTime stopDt = QDateTime::fromString(fields[1], Qt::DateFormat::ISODate);
        if (stopDt.isNull())
            throw std::runtime_error("Unexpected stop-date field value: " + fields[1].toStdString());
        unsigned long stopMeterVal = fields[2].trimmed().toULong(&converted);
        if (!converted)
            throw std::runtime_error("Unexpected stop-meter-value field value: " + fields[2].toStdString());
        int stopReasonFieldVal = fields[3].trimmed().toInt(&converted);
        if (!converted)
            throw std::runtime_error("Unexpected stop-reason field value: " + fields[3].toStdString());
        trans->setFinished(stopDt, stopMeterVal, static_cast<ocpp::Reason>(stopReasonFieldVal));
    }
    catch (std::runtime_error &err) {
        qCWarning(Transactions) << "EvChargingTransaction::fromStringList() error: " << err.what();
        if (trans) {
            trans.reset();
        }
    }

    return trans;
}

void EvChargingTransaction::toJsonObject(QJsonObject &jsonObj){

    jsonObj["authId"] = this->authId;
    jsonObj["startMeterVal"] = QString::number(this->startMeterVal);
    jsonObj["endMeterVal"] = QString::number(this->endMeterVal);
    jsonObj["lastSentMeterValIdx"] = this->lastSentMeterValIdx;

    jsonObj["transactionId"] = QString::number(this->transactionId);
    jsonObj["st"] = static_cast<int>(this->st);
    jsonObj["stopReason"] = static_cast<int>(this->stopReason);
    jsonObj["startDt"] = this->startDt.toString(Qt::DateFormat::ISODate);
    jsonObj["endDt"] = this->endDt.toString(Qt::DateFormat::ISODate);

    QJsonArray jsonMeters;
    foreach(const EvChargingDetailMeterVal &item, this->meterValUpdates){
        QJsonObject in_Obj;
        //EvChargingDetailMeterVal &item = meterValUpdates[j];
        in_Obj["meteredDt"] = item.meteredDt.toString(Qt::DateFormat::ISODate);
        in_Obj["Q"] = QString::number(item.Q);
        in_Obj["P"] = QString::number(item.P);
        in_Obj["Pf"] = QString::number(item.Pf);
        in_Obj["Freq"] = QString::number(item.Freq);
        QJsonArray Amp,Voltage;
        for(int i=0;i<3;i++){
            Amp.append(QString::number(item.I[i]));
            Voltage.append( QString::number(item.V[i]));
        }
        in_Obj["I"] = Amp;
        in_Obj["V"] = Voltage;
        jsonMeters.append(in_Obj);
    }
    jsonObj["meterValUpdates"] = jsonMeters;
}

QString EvChargingTransaction::getTransactionId()
{
    return QString::number(transactionId);
}

long EvChargingTransaction::getTransactionId2()
{
    return transactionId;
}
