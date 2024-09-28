#ifndef CCHARGER_H
#define CCHARGER_H
#include <QObject>
#include <QtCore/QString>
#include <QUdpSocket>
//#include <QtGlobal>
#include <QDateTime>
#include "T_Type.h"
#include "evchargingtransaction.h"
#include "OCPP/ocppTypes.h"
#include "transactionrecordfile.h"
#include "powermeterrecfile.h"
#include <qtimer.h>

class CCharger : public QObject
{
    Q_OBJECT
public:
    static constexpr int MeterDecPts = 1;
    static double meterValToWh(int meterVal) { return meterVal * 100.0; /* SPM9x MeterVal is x0.1 kWH */ }
    static int meterValSubtract(int startVal, int endVal); // takes care of meter value rollover
    static CCharger &instance() { static CCharger charger; return charger; }
public:
    enum class OcppServerReqType {
        Nil,
        Authorize,
        DataTransferTimeValue,
    };
    enum class ConnectorType : unsigned char
    {
        EvConnector = 0, // IEC2
        WallSocket = 1, // 13A
        AC63A_NoLock = 2, // 63A No-Connector-Lock
    };

    enum class Response : int {
        OK = 0,
        Busy = 1,
        Denied = 2
    };
public:
    bool IsPreparing();
    bool IsCharging();
    bool IsChargingOrSuspended();
    bool IsResumedCharging() { return isResumedCharging; }
    bool IsFinishing();
    bool IsBooking();
    bool IsUnLock();
    void UnLock();
    void Lock();
    void StartCharge();
    void ResumeCharge();
    void ChargeDateTime();
    void StopCharge();
    void ChargerIdle();
    bool IsCablePlugIn();
    bool IsEVConnected();
    void Refresh();
    ocpp::ChargerStatus  StatusManage();
    bool IsFaultState();
    bool IsPreparingState();
    bool IsSuspendedStartEVSE();
    bool IsSuspendedEVorEVSE();
    bool IsChargeState();
    bool IsIdleState();
    bool IsUnnormalStop();
    ocpp::ChargePointErrorCode GetErrorCode();
    bool NewRecord();
    bool ModifyRecord();
    bool ReadRecord();
    void ClearRecord();
    void CreateReserveRecord();
    void ReadReserveRecord();
    void DeleteReserveRecord();
    void SendUdpLog(const QByteArray &data);
    bool IsBookingState();
    void SetBooking();
    void CancelBooking();
    void SaveCurrentChargeRecord(TransactionRecordFile &recordFile);
    void SavePowerMeterRecord(PowerMeterRecFile &recordFile);
    void SaveMsgRecord(QString newmsg);
    void SoftPCB();
    void StartChargeToFull();
    bool IsChargingToFull();
    void SetConnType(ConnectorType c) { connType = c; }
    ConnectorType ConnType() { return connType; };
    void SetProfileCurrent(int maxCurrent);
    void SetDefaultProfileCurrent(int maxCurrent); // in Amps
    int  GetCableMaxCurrent() { return cableMaxCurrent; };
    int GetCurrentLimit() { return connAvailable? std::min(profileCurrent, maxCutOffCurrent) : 0; };
    Response ChangeConnAvail(bool newAvailVal);
    bool IsConnectorAvailable() { return connAvailable; }
    EvChargingDetailMeterVal GetMeterVal();

    // Development purpose only
    void setFakeCurrentOffset(long newOffset);
    void accumChargedQBeforeSusp(u32 bf);
    u32 getTotalChargedQ();

signals:
    void errorCodeChanged();    // Charger::ErrorCode field
    void stopChargeRequested();
    void connAvailabilityChanged(bool newAvail);
    // transId is only relevant when isResumedCharging is true (but still could be invalid because of offline-charging)
    void chargeCommandAccepted(QDateTime startDt, unsigned long startMeterVal, bool isResumedChanging, int transId);
    void chargeCommandCanceled(QDateTime canceledDt, unsigned long cancelMeterVal, ocpp::Reason cancelReason);
    void chargeFinished(QDateTime stopDt, unsigned long stopMeterVal, ocpp::Reason stopReason);

    //stop resume charge transaction
    void stopResumeChargeTransaction(const QString transId, const QDateTime stopDt, const unsigned long stopMeterVal, const ocpp::Reason reason, const QString tagId);

public slots:
    void ChargerModBus(const QByteArray &data);
    void ChargerModBus2(const CHARGER_RAW_DATA &data);
    void onQueueingCountRemainingTime();

public:
    int  ConnectTimeout;    //MODBUS通信连接
    bool Initialized;       //已从充电板中读到数据
    uint LockTimeout;
    uint CommandTimeout;    //充、停电命令响应限时  

    ocpp::ChargerStatus     Status;             //充电进度状态
    ocpp::ChargerStatus     OldStatus;
    ulong State;            //充电板状态
    ulong RawState;

    //Task:
    char Command;           //发给充电板的指令
    ulong ChargeTimeValue;  //充电时间设定值 sec
    ulong ChargeQValue;     //充电电量设定值 KWh
    ulong ChargedQ;         //已充电电量
    ulong RemainTime;       //剩余时间
    ulong MeterStart;
    ulong MeterStop;
    QDateTime StartDt;      // in UTC-0
    QDateTime StopDt;       // in UTC-0
    QString IdTag;

    ocpp::Reason StopReason;

    QString VendorErrorCode;    //发送到故障窗口的故障码

    //Booking:
    int         ReservationId;
    QString     ResIdTag;
    QDateTime   ExpiryDate;
    bool bookingState;

    QString     OldMsg;
    QString     Password;       //充电密码    

    QString popupMsgTitlezh;
    QString popupMsgTitleen;
    QString popupMsgContentzh;
    QString popupMsgContenten;
    uint popupMsgContentFontSize;
    QString popupMsgContentColor;
    QString popupMsgTitleColor;
    ulong popupMsgDuration;
    bool popupIsQueueingMode;
    bool queueingWaiting;
    constexpr static ulong ChargeToFullTimeSecs = 100*60*60;
    QTimer *queueingCountRemainingTime;
    ulong chargedTime;

private:
    const int maxCutOffCurrent;     //设定的限流值
    ConnectorType connType;
    const int maxNumMsgRecords;
    const int minNumMsgRecordsToKeep;
    const QString chrgProgressCsvPath;
    const QString chrgProgressCsvPathBackup;
    const QString reserveRecordPath;
    bool chrgProgressCsvPathfilecontrol = true;
    bool        isResumedCharging; // Started by ReadRecord()
    int         resumedTransactionId;
    long fakeLoadCurrent;
    int cableMaxCurrent;

    int profileCurrent;             // Set by ChargingProfile for this transaction
    int defaultProfileCurrent;      // Set by ChargingProfile.chargingProfilePurpose = TxDefaultProfile
    int appliedMaxCurrentSetting;   // Max-current actually applied and reported by the charger-board.

    bool    connAvailable;       // Connector-Availability: See OCPP ChangeAvailability
    const QString connAvailSettingPath;

    CHARGER_RAW_DATA ChargerNewRaw;
    u32 accumulatedChargedQ = 0;

    ocpp::ChargePointErrorCode errorCode;

    bool stopChargeFlag;

    QByteArray preRecvData;
    //int contactorErrorCounter =0;
private:
    CCharger();
    long fakeLoadEnergy();
    long getFakeCurrentOffset();
    void applyDefaultCurrent();
    ocpp::ChargerStatus deduceBeforeChargeSuspendStatus();
    ocpp::ChargerStatus deduceChargeSuspendStatus(); // Charging in-progress, but could be suspended EV(SE)
    bool saveConnAvailabilityToFile(bool newCnnAvlVal);
    void loadConnAvailabilityFromFile();
    bool isIdleState(unsigned long chargerStateValue);
    bool isChargeState(unsigned long chargerStateValue);
    bool isFinishState(unsigned long chargerStateValue);
    bool isCablePlugIn(int cableMxCurrent);
    bool isEvConnected(unsigned long chargerStateVal);
    ocpp::ChargePointErrorCode extractErrorCodeFromState();

    void ChargerPostModBus();


};

#endif // CCHARGER_H
