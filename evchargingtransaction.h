#ifndef EVCHARGINGTRANSACTION_H
#define EVCHARGINGTRANSACTION_H

#include <QDateTime>
#include <QStringList>
#include <QSharedPointer>
#include "OCPP/ocppTypes.h"
#include "T_Type.h"
#include "transactionrecordfile.h"

class EvChargingTransaction
{
public:
    // Serialization
    static QSharedPointer<EvChargingTransaction> fromStringList(const QStringList lines);
    // used in from/toStringList()
    enum class RecordLineType : int {
        TransStart = 1,
        MeterValueUpdate = 2,
        TransStop = 3
    };

public:
    EvChargingTransaction(const QDateTime startDt, const ulong startMeterVal, const QString authorizedIdToken = QString());

    // Serialization
    QStringList toStringList();

    bool hasAuthorizationId() { return !(authId.isNull() || authId.isEmpty()); }
    QString getAuthorizationId() { return authId; };
    // meterValIdx should only be returned for the first transaction.
    // Case: Transaction started online, server went offline before charging finishes.
    //       In case the system restarted, should not re-send MeterValues that has already been sent.
    void setTransactionId(const QString transId, const int lastSentMeterValIdx = -1);
    void updateLastSentMeterValIdx(const int idx) { lastSentMeterValIdx = idx; }
    bool hasTransactionId() { return (transactionId >= 0); }
    QDateTime getTransactionStartDateTime() { return startDt; }
    unsigned long getTransactionStartMeterQValue() { return startMeterVal; }
    QDateTime getTransactionEndDateTime() { return endDt; }
    unsigned long getTransactionEndMeterQValue() { return endMeterVal; }
    ocpp::Reason getTransactionEndReason() { return stopReason; }
    QString getTransactionId();
    long getTransactionId2();
    void setActive();
    bool isActive() { return st == Status::Active; }
    void updateMeterVal(EvChargingDetailMeterVal meterVal);
    int getNumMeterVals() { return meterValUpdates.count(); }
    bool hasSentMeterValIdx() { return lastSentMeterValIdx >= 0; }
    int getLastSentMeterValIdx() { return lastSentMeterValIdx; }
    EvChargingDetailMeterVal getMeterVal(int idx, bool *ok = nullptr);
    void setCanceled(const QDateTime cancelDt, const ulong cancelMeterVal, const ocpp::Reason cancelReason); // No charging happened
    void setFinished(const QDateTime endDt, const ulong endMeterVal, const ocpp::Reason stopReason);
    void saveToFile(TransactionRecordFile &recFile);
    void toJsonObject(QJsonObject &json);

    static QSharedPointer<EvChargingTransaction> currentTransc;

private:
    enum class Status : int {
        Unknown,
        Active,
        Canceled,
        Finished
    };
private:
    Status st;
    QDateTime startDt;
    QDateTime endDt;
    unsigned long startMeterVal;
    unsigned long endMeterVal;
    QVector<EvChargingDetailMeterVal> meterValUpdates;
    ocpp::Reason stopReason;
    long transactionId;
    QString authId;
    int lastSentMeterValIdx;
};

#endif // EVCHARGINGTRANSACTION_H
