#ifndef EVCHARGINGTRANSACTIONREGISTRY_H
#define EVCHARGINGTRANSACTIONREGISTRY_H

#include <QObject>
#include <QSharedPointer>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaObject>
#include <QMetaProperty>
#include <qobjectdefs.h>
#include "evchargingtransaction.h"
#include "transactionrecordfile.h"

///
/// \brief The EvChargingTransactionRegistry class
///
/// \abstract Upstream items such as MainWindow will call the methods directly.
///           Downsream items such as OcppClient and Transaction-Taker (TBD) will
///           be connecting to the QT Signals of this class.
///           'Active' transaction is the one that is actually going on in real-time.
///           Transaction being communicated with CS over OCPP could be present or past
///           Charging 'transactions.
///
class EvChargingTransactionRegistry : public QObject
{
    Q_OBJECT
public:
    static EvChargingTransactionRegistry &instance() { static EvChargingTransactionRegistry registry; return registry; }

    void setMeterSampleInterval(std::chrono::seconds period);
    void startNew(QDateTime startDt, unsigned long startMeterVal, const QString idTag = QString());
    // Add an already ongoing transaction to registry
    void addActive(QDateTime startDt, unsigned long startMeterVal, const QString idTag = QString(), QVector<EvChargingDetailMeterVal> meterVals = {}, QString transId = QString());
    void updateTransactionId(const QString idTag, const QString transId);
    void cancel(QDateTime cancelDt, unsigned long cancelMeterVal, ocpp::Reason cancelReason);
    void finish(QDateTime stopDt, unsigned long stopMeterVal, ocpp::Reason stopReason);
    void remove(QSharedPointer<EvChargingTransaction> trans);
    bool removeByAuthId(const QString idTag);
    bool removeByTransactionId(const QString transId);
    bool hasTransactionId(const QString transId);
    QSharedPointer<EvChargingTransaction> getNewest();
    QSharedPointer<EvChargingTransaction> getOldest();
    bool hasActiveTransaction();
    bool activeTransactionHasTransId();
    QString getActiveTransactionId();
    bool isActiveTransactionId(const QString transId);
    int getNumNonActiveTransactions();
    void saveTransactionToRecord(const QString transId, TransactionRecordFile &recFile);

signals:
    void chargingStarted(const QSharedPointer<EvChargingTransaction>); // New Transaction added without Transaction-ID
    void chargingResumed(const QSharedPointer<EvChargingTransaction>); // Previous Transaction resumed with existing Transaction-ID
    void meterValUpdated(const QSharedPointer<EvChargingTransaction>); // Meter Value updated
    void chargingCanceled(const QSharedPointer<EvChargingTransaction>); // Transaction canceled before becoming Active
    void chargingFinished(const QSharedPointer<EvChargingTransaction>); // Transaction finished

private:
    QVector<QSharedPointer<EvChargingTransaction>> transItems;
    const int defaultSamplePeriodSecs = 10;
    std::chrono::seconds samplePeriod;
    QTimer meterSampleTimer;
    const QString offlineTransFileName;

    void startSampleIfNeeded();
    void updateActiveTransactionMeterValue();
    QSharedPointer<EvChargingTransaction> getActiveTrans();

    // Serialization - save completed offline-transactions to file
    void loadOfflineTransactionsFromFile(); // called by class=constructor
    void saveOfflineTransactionsToFile(); // called by remove()
    void saveOfflineTransactionsToJSON();
    void saveOfflineTransactionsToJSON(QSharedPointer<EvChargingTransaction> trans);
    void addOfflineTransactionsToFile(QSharedPointer<EvChargingTransaction>); // called by finish()

private slots:
    void onMeterSampleTimeout();

private:
    explicit EvChargingTransactionRegistry(QObject *parent = nullptr);
};

#endif // EVCHARGINGTRANSACTIONREGISTRY_H
