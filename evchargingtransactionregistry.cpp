#include <QSaveFile>
#include "evchargingtransactionregistry.h"
#include "evchargingtransaction.h"
#include "logconfig.h"
#include "ccharger.h"
#include "gvar.h"
#include "hostplatform.h"


EvChargingTransactionRegistry::EvChargingTransactionRegistry(QObject *parent) :
    QObject(parent),
    samplePeriod(defaultSamplePeriodSecs),
    offlineTransFileName("offline_transaction_records.csv")
{
    loadOfflineTransactionsFromFile();

    meterSampleTimer.setInterval(samplePeriod);
    meterSampleTimer.setSingleShot(false);
    connect(&meterSampleTimer, &QTimer::timeout, this, &EvChargingTransactionRegistry::onMeterSampleTimeout);
}

void EvChargingTransactionRegistry::setMeterSampleInterval(std::chrono::seconds period)
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Set meter sample interval";
    // adjust an active timer if necessary
    bool needResume = false;

    if (meterSampleTimer.isActive()) {
        if (samplePeriod != period) {
            meterSampleTimer.stop();
            needResume = true;
        }
    }
    samplePeriod = period;
    meterSampleTimer.setInterval(samplePeriod);
    if (needResume)
        meterSampleTimer.start();
}

void EvChargingTransactionRegistry::startNew(QDateTime startDt, unsigned long startMeterVal, const QString idTag)
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Start new. AuthId: " << idTag;
    if (idTag.isNull() || idTag.isEmpty()) {
        qCWarning(Transactions) << "startNew() with null or empty idTag";
    }
    //QSharedPointer<EvChargingTransaction> newTrans = QSharedPointer<EvChargingTransaction>::create(startDt, startMeterVal, idTag);
    EvChargingTransaction::currentTransc = QSharedPointer<EvChargingTransaction>(new EvChargingTransaction(startDt, startMeterVal, idTag));
    transItems.append(EvChargingTransaction::currentTransc);
    EvChargingTransaction::currentTransc->setActive();
    startSampleIfNeeded();
    //EvChargingTransaction::currentTransc = transcPtr;
    emit chargingStarted(EvChargingTransaction::currentTransc);
}

// Add an already ongoing transaction to registry
void EvChargingTransactionRegistry::addActive(QDateTime startDt, unsigned long startMeterVal, const QString idTag, QVector<EvChargingDetailMeterVal> meterVals, QString transId)
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Add active. Transaction ID: " << transId << " AuthId: " << idTag;
    if (idTag.isNull() || idTag.isEmpty()) {
        qCWarning(Transactions) << "addActive() with null or empty idTag";
    }
    //QSharedPointer<EvChargingTransaction> newTrans = QSharedPointer<EvChargingTransaction>::create(startDt, startMeterVal, idTag);
    auto transcPtr = QSharedPointer<EvChargingTransaction>(new EvChargingTransaction(startDt, startMeterVal, idTag));
    transcPtr->setActive();
    if (!(transId.isNull() || transId.isEmpty()))
        transcPtr->setTransactionId(transId);
    for (auto itr = meterVals.constBegin(); itr != meterVals.constEnd(); itr++)
        transcPtr->updateMeterVal(*itr);
    transItems.append(transcPtr);
    startSampleIfNeeded();
    emit chargingResumed(transcPtr);
}


void EvChargingTransactionRegistry::cancel(QDateTime cancelDt, unsigned long cancelMeterVal, ocpp::Reason cancelReason)
{
    QSharedPointer<EvChargingTransaction> activeTrans = this->getNewest();

    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Cancel. Reason: " << ocpp::reasonStrs.at(cancelReason).data() <<  "Transaction id: " << activeTrans->getTransactionId2() << " AuthId: " << activeTrans->getAuthorizationId();

    Q_ASSERT(activeTrans->isActive());
    if (activeTrans->isActive()) {
        meterSampleTimer.stop();
        activeTrans->setCanceled(cancelDt, cancelMeterVal, cancelReason);
        qCDebug(Transactions) << "Transaction of Id " << activeTrans->getTransactionId2() << " marked canceled";
        emit chargingCanceled(activeTrans);
    }
    else {
        qCWarning(Transactions) << "Unexpected request to cancel an non-active transaction of Id " << activeTrans->getTransactionId2() << " in registry";
    }
}

// Mark the currently active transaction as finish
void EvChargingTransactionRegistry::finish(QDateTime stopDt, unsigned long stopMeterVal, ocpp::Reason stopReason)
{
    qWarning()<<"Finished EvChargingTransaction:"<<EvChargingTransaction::currentTransc<<((EvChargingTransaction::currentTransc==nullptr)?-1:EvChargingTransaction::currentTransc->getTransactionId2());
    //QSharedPointer<EvChargingTransaction> activeTrans = this->getNewest();
    if(EvChargingTransaction::currentTransc == nullptr) return;

    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Finish. Reason: " << ocpp::reasonStrs.at(stopReason).data() <<  "Transaction id: " << EvChargingTransaction::currentTransc->getTransactionId2() << " AuthId: " << EvChargingTransaction::currentTransc->getAuthorizationId();

    //Q_ASSERT(activeTrans->isActive());
    if (EvChargingTransaction::currentTransc->isActive()) {
        meterSampleTimer.stop();
        EvChargingTransaction::currentTransc->setFinished(stopDt, stopMeterVal, stopReason);
        qCDebug(Transactions) << "Transaction of Id " << EvChargingTransaction::currentTransc->getTransactionId2() << " marked finished";
        saveOfflineTransactionsToJSON(EvChargingTransaction::currentTransc);
        addOfflineTransactionsToFile(EvChargingTransaction::currentTransc);
        emit chargingFinished(EvChargingTransaction::currentTransc);
    }
    else {
        qCWarning(Transactions) << "Unexpected request to finish an non-active transaction of Id " << EvChargingTransaction::currentTransc->getTransactionId2() << " in registry";
    }
}

void EvChargingTransactionRegistry::remove(QSharedPointer<EvChargingTransaction> trans)
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Remove. Transaction id: " << trans->getTransactionId2() << " AuthId: " << trans->getAuthorizationId();

    int i = transItems.indexOf(trans);
    if (i >= 0) {
        if (trans->isActive())
            qCDebug(Transactions()) << "Removing active Transaction of Id " << trans->getTransactionId2();
        trans.reset();
        transItems.removeAt(i);
        qCDebug(Transactions) << "Transaction of Id " << trans->getTransactionId2() << " removed successfully";
        saveOfflineTransactionsToFile();
        //saveOfflineTransactionsToJSON();
    }
    // there shouldn't be another one
    do {
        i = transItems.indexOf(trans);
        qCWarning(Transactions) << "More than one occurrence of Transaction of Id " << trans->getTransactionId2() << " in registry";
    } while (i >= 0);
}

bool EvChargingTransactionRegistry::removeByAuthId(const QString idTag)
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Remove by authId. AuthId: " << idTag;

    bool removed = false;
    for (auto itr = transItems.begin(); itr != transItems.end(); itr++) {
        QSharedPointer<EvChargingTransaction> trans = *itr;
        if (trans->hasAuthorizationId()) {
            if (trans->getAuthorizationId().compare(idTag) == 0) {
                trans.reset();
                itr = transItems.erase(itr);
                removed = true;
                break;
            }
        }
    }
    if (removed){
        saveOfflineTransactionsToFile();
        //saveOfflineTransactionsToJSON();
    }
    return removed;
}

bool EvChargingTransactionRegistry::removeByTransactionId(const QString transId)
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Remove by transaction id. Transaction id: " << transId;

    bool removed = false;
    for (auto itr = transItems.begin(); itr != transItems.end(); itr++) {
        QSharedPointer<EvChargingTransaction> trans = *itr;
        if (trans->hasTransactionId()) {
            if (trans->getTransactionId().compare(transId) == 0) {
                trans.reset();
                itr = transItems.erase(itr);
                removed = true;
                break;
            }
        }
    }
    if (removed){
        saveOfflineTransactionsToFile();
        //saveOfflineTransactionsToJSON();
    }
    return removed;
}

void EvChargingTransactionRegistry::saveTransactionToRecord(const QString transId, TransactionRecordFile &recFile)
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Save transaction to record. Transaction id: " << transId;

    for (auto itr = transItems.begin(); itr != transItems.end(); itr++) {
        QSharedPointer<EvChargingTransaction> trans = *itr;
        if (trans->hasTransactionId()) {
            if (trans->getTransactionId().compare(transId) == 0) {
                itr->get()->saveToFile(recFile);
                break;
            }
        }
    }
}

void EvChargingTransactionRegistry::saveOfflineTransactionsToJSON()
{
    QJsonArray jsonObj;
    for (QSharedPointer<EvChargingTransaction> obj : transItems) {
        QJsonObject jObj;
        obj->toJsonObject(jObj);
        jsonObj.append(jObj);
    }
    QJsonDocument jsonDoc(jsonObj);
    QString jsonString = jsonDoc.toJson(QJsonDocument::Indented);

    qWarning() << jsonString;
    QString fullPath = QString("%1%2").arg(charger::Platform::instance().recordsDir)
            .arg(QString("offline_transaction_%1.json").arg(QDateTime().currentDateTime().toString("yyMMddhhmmss")));
    QSaveFile file(fullPath);
    try {
        qCDebug(Transactions) << "Saving offline-transactions to file";
        if (!file.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::OpenModeFlag::Truncate))
            throw std::runtime_error(fullPath.toStdString() + " open-failed: " + file.errorString().toStdString());
        QTextStream out(&file);
        out << jsonString;
        file.commit(); // can only commit once
    }
    catch (std::runtime_error &err) {
        qCWarning(Transactions) << "EvChargingTransactionRegistry::saveOfflineTransactionsToFile() error: " << err.what();
        if (file.isOpen()) {
            file.cancelWriting();
            file.commit();
        }
    }
}

void EvChargingTransactionRegistry::saveOfflineTransactionsToJSON(QSharedPointer<EvChargingTransaction> trans)
{
    QJsonObject jsonObj;
    trans->toJsonObject(jsonObj);
    QJsonDocument jsonDoc(jsonObj);
    QString jsonString = jsonDoc.toJson(QJsonDocument::Compact);

    qWarning() << jsonString;
    QString recordId = QDateTime().currentDateTime().toString("yyMMddhhmmss");
    QString fullPath = QString("%1%2").arg(charger::Platform::instance().recordsDir)
            .arg(QString("offline_transaction_%1.json").arg(recordId));
    QSaveFile file(fullPath);
    try {
        qCDebug(Transactions) << "Saving offline-transactions to file";
        if (!file.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::OpenModeFlag::Truncate))
            throw std::runtime_error(fullPath.toStdString() + " open-failed: " + file.errorString().toStdString());
        QTextStream out(&file);
        out << jsonString;
        file.commit(); // can only commit once
    }
    catch (std::runtime_error &err) {
        qCWarning(Transactions) << "EvChargingTransactionRegistry::saveOfflineTransactionsToFile() error: " << err.what();
        if (file.isOpen()) {
            file.cancelWriting();
            file.commit();
        }
    }
    //interact with shell to pack transaction records
    QString output;
    charger::Platform::asyncShellCommand(QStringList()<<"Charger_cmd.sh"<<"charging"<<"-m"<<recordId,output);
    qDebug()<<output;
}

QSharedPointer<EvChargingTransaction> EvChargingTransactionRegistry::getNewest()
{
    QSharedPointer<EvChargingTransaction> trans;

    if(transItems.length() > 0){
        trans = transItems.last();
    }

    return trans;
}

QSharedPointer<EvChargingTransaction> EvChargingTransactionRegistry::getOldest()
{
    QSharedPointer<EvChargingTransaction> trans;

    if(transItems.length() > 0){
        trans = transItems.first();
    }

    return trans;
}

void EvChargingTransactionRegistry::startSampleIfNeeded()
{
    // Assumption: there is only one active-transaction
    QSharedPointer<EvChargingTransaction> newestTrans = getNewest();
    if (newestTrans && newestTrans->isActive()) {
        if (!meterSampleTimer.isActive()) {
            meterSampleTimer.start();
        }
    }
}

void EvChargingTransactionRegistry::onMeterSampleTimeout()
{
    updateActiveTransactionMeterValue();
}

void EvChargingTransactionRegistry::updateActiveTransactionMeterValue()
{
    QSharedPointer<EvChargingTransaction> newestTrans = getNewest();
    if (newestTrans && newestTrans->isActive()) {
        EvChargingDetailMeterVal meterVal = m_ChargerA.GetMeterVal();
        newestTrans->updateMeterVal(meterVal);
        qCDebug(Transactions) << "Updated meter value to active transaction";
        emit meterValUpdated(newestTrans);
    }
    else {
        qCWarning(Transactions) << "There is no active tranaction in registry to sample charging value.";
    }
}

void EvChargingTransactionRegistry::updateTransactionId(const QString idTag, const QString transId)
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Update transaction id. Transaction id: " << transId << " idTag: " << idTag;

    try {
        if (idTag.isNull() || idTag.isEmpty()) {
            throw new std::invalid_argument("Trying to update transaction-Id to null-value Id-Tag in registry.");
        }
        if (transId.isNull() || transId.isEmpty()) {
            throw new std::invalid_argument("Trying to set null-value transaction-Id Id-Tag " + idTag.toStdString() + " in registry.");
        }
        bool found = false;
        for (auto itr = transItems.begin(); itr != transItems.end(); itr++) {
            if (itr->get()->hasAuthorizationId()) {
                if (transId.compare(itr->get()->getAuthorizationId()) == 0) {
                    itr->get()->setTransactionId(transId);
                    qCDebug(Transactions) << "Updated tranaction to active transaction";
                    found = true;
                }
            }
        }
        if (!found) {
            throw std::runtime_error("There is no active tranaction in registry to sample charging value.");
        }
    }
    catch (std::exception &e) {
        qCWarning(Transactions) << e.what();
    }
}

QSharedPointer<EvChargingTransaction> EvChargingTransactionRegistry::getActiveTrans()
{
    QSharedPointer<EvChargingTransaction> activeTrans;
    // starts from the end (newest)
    for (auto itr = transItems.crbegin(); itr != transItems.crend(); itr++) {
        if (itr->get()->isActive()) {
            activeTrans = QSharedPointer<EvChargingTransaction>(*itr);
        }
    }
    return activeTrans;
}

bool EvChargingTransactionRegistry::hasActiveTransaction()
{
    QSharedPointer<EvChargingTransaction> activeTrans = getActiveTrans();
    return activeTrans;
}

bool EvChargingTransactionRegistry::activeTransactionHasTransId()
{
    QSharedPointer<EvChargingTransaction> activeTrans = getActiveTrans();
    if (activeTrans)
        return activeTrans->hasTransactionId();
    return false;
}

QString EvChargingTransactionRegistry::getActiveTransactionId()
{
    QSharedPointer<EvChargingTransaction> activeTrans = getActiveTrans();
    if (activeTrans && activeTrans->hasTransactionId())
        return activeTrans->getTransactionId();
    return QString();
}

bool EvChargingTransactionRegistry::isActiveTransactionId(const QString transId)
{
    QSharedPointer<EvChargingTransaction> activeTrans = getActiveTrans();
    if (activeTrans && activeTrans->hasTransactionId()) {
        return (transId.compare(activeTrans->getTransactionId()) == 0);
    }
    return false;
}

bool EvChargingTransactionRegistry::hasTransactionId(const QString transId)
{
    bool found = false;
    if (!(transId.isNull() || transId.isEmpty())) {
        for (auto itr = transItems.crbegin(); itr != transItems.crend(); itr++) {
            if (itr->get()->hasTransactionId() && (transId.compare(itr->get()->getTransactionId()) == 0)) {
                found = true;
                break;
            }
        }
    }
    return found;
}

int EvChargingTransactionRegistry::getNumNonActiveTransactions()
{
    int numNonActiveTrans = 0;
    QSharedPointer<EvChargingTransaction> activeTrans;
    // starts from the end (newest)
    for (auto itr = transItems.crbegin(); itr != transItems.crend(); itr++) {
        if (!itr->get()->isActive()) {
            numNonActiveTrans++;
        }
    }

    return numNonActiveTrans;
}

void EvChargingTransactionRegistry::saveOfflineTransactionsToFile()
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Save offline transaction to file.";

    QString fullPath = QString("%1%2").arg(charger::Platform::instance().recordsDir)
            .arg(offlineTransFileName);
    QSaveFile file(fullPath);

    try {
        qCDebug(Transactions) << "Saving offline-transactions to file";
        if (!file.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::OpenModeFlag::Truncate))
            throw std::runtime_error(fullPath.toStdString() + " open-failed: " + file.errorString().toStdString());
        QTextStream out(&file);
        for(auto i = transItems.cbegin(); i != transItems.cend(); i++) {
            QStringList lines = i->get()->toStringList();
            if (!lines.isEmpty()) {
                for (auto j = lines.cbegin(); j != lines.cend(); j++) {
                    out << *j << "\n";
                    if (out.status() != QTextStream::Ok || file.error() != QFileDevice::FileError::NoError) {
                        throw std::runtime_error("Error detected writing transaction to file. Abort. " + file.errorString().toStdString());
                    }
                }
            }
        }
        file.commit(); // can only commit once
    }
    catch (std::runtime_error &err) {
        qCWarning(Transactions) << "EvChargingTransactionRegistry::saveOfflineTransactionsToFile() error: " << err.what();
        if (file.isOpen()) {
            file.cancelWriting();
            file.commit();
        }
    }
}

void EvChargingTransactionRegistry::addOfflineTransactionsToFile(QSharedPointer<EvChargingTransaction> trans)
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Add offline transaction to file.";

    QString fullPath = QString("%1%2").arg(charger::Platform::instance().recordsDir)
            .arg(offlineTransFileName);
    QFile file(fullPath);

    try {
        qCDebug(Transactions) << "Adding an offline-transaction of start-date"
                              << trans->getTransactionStartDateTime().toLocalTime().toString(Qt::DateFormat::LocalDate)
                              << "to file";
        if (!file.open(QIODevice::OpenModeFlag::WriteOnly|QIODevice::OpenModeFlag::Append)) // TODO: QSaveFile not supporting Append yet
            throw std::runtime_error(fullPath.toStdString() + " open-failed: " + file.errorString().toStdString());
        QTextStream out(&file);
        QStringList lines = trans->toStringList();
        if (!lines.isEmpty()) {
            for (auto j = lines.cbegin(); j != lines.cend(); j++) {
                out << *j << "\n";
                if (out.status() != QTextStream::Ok || file.error() != QFileDevice::FileError::NoError) {
                    throw std::runtime_error("Error detected writing transaction to file. Abort. " + file.errorString().toStdString());
                }
            }
        }
        file.close();
    }
    catch (std::runtime_error &err) {
        qCWarning(Transactions) << "EvChargingTransactionRegistry::saveOfflineTransactionsToFile() error: " << err.what();
        if (file.isOpen()) {
            file.close();
        }
    }
}


void EvChargingTransactionRegistry::loadOfflineTransactionsFromFile()
{
    qWarning() << "EV CHARGING TRANSACTION REGISTRY: Load offline transaction from file";

    QString fullPath = QString("%1%2").arg(charger::Platform::instance().recordsDir)
            .arg(offlineTransFileName);
    QFile file(fullPath);

    using RecordLineType = EvChargingTransaction::RecordLineType;
    try {
        qCDebug(Transactions) << "Loading offline-transactions from file";
        if (!file.open(QIODevice::OpenModeFlag::ReadOnly))
            throw std::runtime_error(fullPath.toStdString() + " open-failed: " + file.errorString().toStdString());
        QTextStream in(&file);
        QString line;
        QStringList lines;
        do {
            line = in.readLine();
            if (!line.isEmpty()) {
                // detect type by reading the first field
                QStringList fields = line.split(',');
                if (fields.count() < 1)
                    throw std::runtime_error("There are not fields in this line: " + line.toStdString());
                bool converted = false;
                int recType = fields[0].toInt(&converted);
                if (!converted)
                    throw std::runtime_error("Unexpected record-line-type value: " + fields[0].toStdString());
                switch (static_cast<RecordLineType>(recType)) {
                case RecordLineType::TransStart:
                    if (!lines.isEmpty())
                        throw std::runtime_error("Abort - Transaction-Start record is not the beginning line.");
                    lines.append(line);
                    break;
                case RecordLineType::MeterValueUpdate:
                    if (lines.isEmpty())
                        throw std::runtime_error("Abort - MeterValue record should not be the beginning line.");
                    lines.append(line);
                    break;
                case RecordLineType::TransStop:
                    if (lines.isEmpty())
                        throw std::runtime_error("Abort - Transaction-Stop record should not be the beginning line.");
                    lines.append(line);
                    QSharedPointer<EvChargingTransaction> newTrans = EvChargingTransaction::fromStringList(lines);
                    if (newTrans) {
                        transItems.append(newTrans);
                        qCDebug(Transactions) << "Loaded offline-transaction with start-date: " << newTrans->getTransactionStartDateTime().toLocalTime().toString(Qt::DateFormat::LocalDate);
                    }
                    lines.clear(); // restart and expects the next TransStart record.
                    break;
                }
            }
        } while (!line.isNull() && !in.atEnd());
        file.close();
    }
    catch (std::runtime_error &err) {
        qCWarning(Transactions) << "EvChargingTransactionRegistry::loadOfflineTransactionsFromFile() error: " << err.what();
        if (file.isOpen()) {
            file.close();
        }
    }
}
