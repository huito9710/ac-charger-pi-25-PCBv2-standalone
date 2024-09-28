#include "ccharger.h"
#include "const.h"
#include <QtCore/QDebug>
#include <QDateTime>
#include <QFile>
#include <QMainWindow>
#include "api_function.h"
#include "hostplatform.h"
#include "chargerconfig.h"
#include "gvar.h"
#include "logconfig.h"

#define  _NO_PCB_TEST 0     //0: normal run

extern QMainWindow *pCurWin;

int CCharger::meterValSubtract(int startVal, int endVal)
{
    int dist = 0;
    if (endVal < startVal) {
        // rollover
        auto const maxMeterVal = (charger::getMeterType() == charger::AcMeterPhase::SPM93)?
                    99999999 : 9999999;
        dist = endVal + maxMeterVal - startVal;
    }
    else
        dist = endVal - startVal;
    return dist;
}

CCharger::CCharger():
    Status(ocpp::ChargerStatus::Available),
    State(0),
    StartDt(),
    StopDt(),
    StopReason(ocpp::Reason::Local),
    maxCutOffCurrent(charger::getMaxCutoffChargeCurrent()),
    connType(ConnectorType::EvConnector),
    maxNumMsgRecords(100000),
    minNumMsgRecordsToKeep(500),
    chrgProgressCsvPath(QString("%1ChargeRecord.$").arg(charger::Platform::instance().recordsDir)),
    chrgProgressCsvPathBackup(QString("%1ChargeRecordBackup.$").arg(charger::Platform::instance().recordsDir)),
    reserveRecordPath(QString("%1ReserveRecord.$").arg(charger::Platform::instance().recordsDir)),
    isResumedCharging(false),
    resumedTransactionId(-1),
    fakeLoadCurrent(5000),
    cableMaxCurrent(0),
    profileCurrent(maxCutOffCurrent),
    defaultProfileCurrent(-1),
    connAvailable(false),
    connAvailSettingPath(QString("%1conn.unavailable").arg(charger::Platform::instance().connAvailabilityInfoDir)),
    errorCode(ocpp::ChargePointErrorCode::InternalError),
    preRecvData(1,0x00)
{
    ConnectTimeout  =0;         //8"
    Initialized     =false;

    Command         =0;
    ChargeTimeValue =0;
    ChargeQValue    =90000;
    ChargedQ        =0;
    RemainTime      =0;
    MeterStart      =0;
    MeterStop       =0;
    IdTag           ="";

    OldMsg          ="";

    loadConnAvailabilityFromFile();
}

bool CCharger::IsPreparing()
{
    static bool lastStatus = false;
    bool thisStatus = (Status ==ocpp::ChargerStatus::Preparing);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is prepareing: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::IsSuspendedEVorEVSE()
{
    bool condSatisfied = false;
    switch (connType) {
    case ConnectorType::EvConnector:
    case ConnectorType::AC63A_NoLock:
        switch (Status) {
        case ocpp::ChargerStatus::SuspendedEV:
        case ocpp::ChargerStatus::SuspendedEVSE:
            condSatisfied = true;
            break;
        default:
            condSatisfied = false;
            break;
        }
        break;
    case ConnectorType::WallSocket:
        condSatisfied = false; // irrelevant
        break;
    }

    static bool lastStatus = false;
    bool thisStatus = condSatisfied;
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is suspended EV or EVSE: " << result;
        lastStatus = thisStatus;
    }

    return condSatisfied;
}

bool CCharger::IsCharging()
{
    static bool lastStatus = false;
    bool thisStatus = (Status ==ocpp::ChargerStatus::Charging);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is Charging: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::IsChargingOrSuspended()
{
    bool condSatisfied = false;
    switch (Status) {
    case ocpp::ChargerStatus::Charging:
    case ocpp::ChargerStatus::SuspendedEV:
    case ocpp::ChargerStatus::SuspendedEVSE:
        condSatisfied = true;
        break;
    default:
        condSatisfied = false;
        break;
    }

    static bool lastStatus = false;
    bool thisStatus = condSatisfied;
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is charging or suspended: " << result;
        lastStatus = thisStatus;
    }

    return condSatisfied;
}

bool CCharger::IsFinishing()
{
    static bool lastStatus = false;
    bool thisStatus = (Status ==ocpp::ChargerStatus::Finishing);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is finishing: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::isFinishState(unsigned long chargerStateValue)
{
    static bool lastStatus = false;
    bool thisStatus = (((chargerStateValue>>16) & 0x0F) == _C_STATE_FINISH);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is finishing state: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::IsBooking()
{
    static bool lastStatus = false;
    bool thisStatus = (Status ==ocpp::ChargerStatus::Reserved);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is booking: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::IsUnLock()
{
    static bool lastStatus = false;
    bool thisStatus = !(State &_C_STATE_LOCK);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is unlock: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

void CCharger::UnLock()
{ //0X:无指令 10:开锁 11:上锁
    qWarning() << "CCHARGER: Unlock";
    if (charger::isConnLockEnabled()) {
        Command &=~_C_LOCKCOMMAND;
        Command |=_C_LOCKCTRL;
        LockTimeout =0;
        qDebug() <<"UnLock command";
    }
    else {
        qDebug() << "UnLock command ignored: Connector-Lock not enabled.";
    }
}

void CCharger::Lock()
{
    qWarning() << "CCHARGER: Lock";
    if (charger::isConnLockEnabled()) {
        Command |=_C_LOCKCOMMAND;
        Command |=_C_LOCKCTRL;
        LockTimeout =0;
        qDebug() <<"Lock command";
    }
    else {
        qDebug() <<"Lock command ignored: Connector-Lock not enabled.";
    }
}

void CCharger::StartCharge()
{
    qWarning() << "CCHARGER: Start charge";
    if(charger::GetQueueingMode())
    {
        queueingWaiting = true;
        RemainTime = ChargeToFullTimeSecs;
        chargedTime = 0;
    }
    Command =_C_CHARGECOMMAND;  // |_C_LOCKCOMMAND |_C_LOCKCTRL;
    CommandTimeout =0;
    LockTimeout    =0;
    StopDt  = QDateTime();
    StopReason = ocpp::Reason::Local;
    if (! IsResumedCharging() ) {
        MeterStart = ChargedQ;
        StartDt = QDateTime::currentDateTimeUtc();
    }
}

void CCharger::ResumeCharge()
{
    qWarning() << "CCHARGER: Resume charge";
    isResumedCharging = true;
    this->StartCharge();
    emit chargeCommandAccepted(StartDt, MeterStart, IsResumedCharging(), resumedTransactionId);
}


void CCharger::ChargeDateTime()
{
    qWarning() << "CCHARGER: Charge date time";
    Command =_C_CHARGEDT;
    CommandTimeout =0;
}

void CCharger::StopCharge()
{
    isSuspended = 0;
    qWarning() << "CCHARGER: Stop charge";
    char temp;

    temp =Command &(~_C_CHARGEMASK);
    temp |=_C_CHARGESTOP;
    Command =temp;
    CommandTimeout =0;

    stopChargeFlag = true;
    if(queueingCountRemainingTime)
    {
        qWarning() << "Timer is not null/false";
        if(queueingCountRemainingTime->isActive())
        {
            qWarning() << "Timer is active";
            if(chargedTime == 0)
            {
                qWarning() << "charge time is 0";
                chargedTime = ChargeTimeValue - queueingCountRemainingTime->remainingTime()/1000;
            }
            qWarning() << chargedTime;
            queueingCountRemainingTime->stop();
        }
        else
        {
            if(chargedTime <= 10)
            {
                qWarning() << "charge time is 0";
                chargedTime = ChargeTimeValue;
            }
            qWarning() << chargedTime;
        }
    }
    emit stopChargeRequested();
}

void CCharger::ChargerIdle()
{
    qWarning() << "CCHARGER: Charger idle";
    char temp;

    temp =Command &(~_C_CHARGEMASK);
    temp |=_C_CHARGEIDLE;
    Command =temp;
    CommandTimeout =0;

    m_ChargerA.ReservationId = NULL;
    isResumedCharging = false;
    resumedTransactionId = -1;

    bookingState = false;
    DeleteReserveRecord();
}

bool CCharger::IsCablePlugIn()
{
    static bool lastStatus = true;
    bool thisStatus = isCablePlugIn(cableMaxCurrent);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is cable plug in: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::isCablePlugIn(int cableMxCurrent)
{
    return (cableMxCurrent > 0);
}
bool CCharger::IsEVConnected()
{
    static bool lastStatus = false;
    bool thisStatus = isEvConnected(State);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is EV connected: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::isEvConnected(unsigned long chargerStateVal)
{
    uchar cp =(chargerStateVal >>24) &0x0F;    //1:A 2:B 3:C 4:D 5:E 6:F
    return ((cp>=2) && (cp<=4));
}


bool CCharger::IsFaultState()
{
    return false;

#if(0)  //增加CP线故障提示--在PCB中改
    State   &=0xFFFFFFBF;
    uchar cp =(State >>24) &0x0F;    //1:A 2:B 3:C 4:D 5:E 6:F
    if(cp >=5) State |=0x0040;       //CP is E/F state
#endif

    static bool lastStatus = false;
    bool thisStatus = State &_C_STATE_FAULT;

#if(0)    //Skip Contactor error for some 3Phase devices
    if((State & 0x1400)==0x1400) //_C_STATE_CONTACTOR
    {
        contactorErrorCounter++;
        if(contactorErrorCounter>30){
            qWarning() << "CCHARGER: CONTACTOR is fault state.";
            contactorErrorCounter=0;
        }
        thisStatus = State &(_C_STATE_FAULT & ~0x1400); //_C_STATE_CONTACTOR
    }
#endif    //End Skipment

#if(0)
    if((Status == ocpp::ChargerStatus::Charging || Status == ocpp::ChargerStatus::SuspendedEVSE)) //&& !charger::GetStopTransactionOnEVSideDisconnect())
    {
        if(State &_C_STATE_DIODE){
            Status = ocpp::ChargerStatus::SuspendedEV;
            isSuspended = 2;
        }
    }
    if(isSuspended == 2){
        if(!thisStatus){
            isSuspended = 0;
            Status = ocpp::ChargerStatus::Charging;
        }
        else{
            thisStatus = State &(_C_STATE_FAULT & ~_C_STATE_DIODE);
        }
        //qWarning()<<"Charger: this Status:"<<thisStatus;
    }
#endif

    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is fault state: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::IsPreparingState()
{
    static bool lastStatus = false;
    bool thisStatus = (IsCablePlugIn() && IsEVConnected() && (Status !=ocpp::ChargerStatus::Finishing));
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is preparing state: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::IsChargeState()
{
    static bool lastStatus = false;
    bool thisStatus = isChargeState(State);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is charge state: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::isChargeState(unsigned long chargerStateValue)
{//在充电中或充电完成状态:
    return (((chargerStateValue >>16) &0x0F) ==_C_STATE_CHARGEING) |(((chargerStateValue >>16) &0x0F) ==_C_STATE_FINISH);
    //return State &((_C_STATE_CHARGEING |_C_STATE_FINISH)<<16);    //booking error
}

bool CCharger::IsIdleState()
{
    static bool lastStatus = false;
    bool thisStatus = isIdleState(State);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is idle state: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

bool CCharger::isIdleState(unsigned long chargerStateValue)
{
    return !(chargerStateValue &(_C_STATE_ENABLE<<16));
}

bool CCharger::IsUnnormalStop()
{
    static bool lastStatus = false;
    bool thisStatus = (((State >>20)&0x0F) ==2);
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is unnormal stop: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

ocpp::ChargePointErrorCode CCharger::GetErrorCode()
{
    qWarning() << "CCHARGER: Get error code: " << ocpp::chargePointErrorCodeStrs.at(errorCode).data();
    return errorCode;
}

ocpp::ChargePointErrorCode CCharger::extractErrorCodeFromState()
{
    ocpp::ChargePointErrorCode boardErr = ocpp::ChargePointErrorCode::NoError;
    if(State &_C_STATE_EARTH)
        boardErr = ocpp::ChargePointErrorCode::GroundFailure;
    else if(State &_C_STATE_CURRENT)
        boardErr = ocpp::ChargePointErrorCode::OverCurrentFailure;
    else if(State &_C_STATE_VOLTAGE)
        boardErr = ocpp::ChargePointErrorCode::OverVoltage;
    else if(IsFaultState())
        boardErr = ocpp::ChargePointErrorCode::OtherError;

    if(boardErr == ocpp::ChargePointErrorCode::NoError)
        VendorErrorCode ="";
    else
        VendorErrorCode =QString("E-%1").arg(State &_C_STATE_FAULT, 4, 16, QLatin1Char('0'));

    static ocpp::ChargePointErrorCode Lasterror = ocpp::ChargePointErrorCode::NoError;
    if(Lasterror != boardErr)
    {
        qWarning() << "CCHARGER: Extract error code from state: " << ocpp::chargePointErrorCodeStrs.at(boardErr).data();
        Lasterror = boardErr;
    }
    return boardErr;
}

void CCharger::SetProfileCurrent(int maxCurrent)
{
    qWarning() << "CCHARGER: Set profile current: " << maxCurrent;
    if(maxCurrent > 0)
    {
        m_ChargerA.queueingWaiting = false;
/*
        queueingCountRemainingTime = new QTimer();
        connect(queueingCountRemainingTime, SIGNAL(timeout()), this, SLOT(onQueueingCountRemainingTime()));
        queueingCountRemainingTime->start(ChargeTimeValue * 1000);
*/
    }
    profileCurrent = maxCurrent;
    // it will be sent to the Charger Board in the MainWindow::ChargerControl()
}

void CCharger::SetDefaultProfileCurrent(int maxCurrent)
{
    qWarning() << "CCHARGER: Set default profile current: " << maxCurrent;
    defaultProfileCurrent = maxCurrent;
    applyDefaultCurrent();
}

void CCharger::applyDefaultCurrent()
{
    qWarning() << "CCHARGER: Apply default profile current";
    if ((defaultProfileCurrent >= 0) && (! IsCharging())) {
        profileCurrent = defaultProfileCurrent;
        // it will be sent to the Charger Board in the MainWindow::ChargerControl()
    }
}

//
// Status: Charging / SuspendedEV / SuspendedEVSE
//
ocpp::ChargerStatus CCharger::deduceChargeSuspendStatus()
{
    Q_ASSERT(((ChargerNewRaw.State >> 16) & 0x0F) == 0x1); // 'Charging' == 1

    using namespace ocpp;
    ChargerStatus st = ChargerStatus::SuspendedEVSE; // default
    switch (connType) {
    case ConnectorType::EvConnector:
    case ConnectorType::AC63A_NoLock:
        switch ((ChargerNewRaw.State >> 24) & 0x0F) {
        case _OCPP_STATE_B:
            st = (appliedMaxCurrentSetting > 0)? ChargerStatus::SuspendedEV : ChargerStatus::SuspendedEVSE;
            break;
        case _OCPP_STATE_C:
            st = (appliedMaxCurrentSetting > 0)? ChargerStatus::Charging : ChargerStatus::SuspendedEVSE;
            break;
        default:
            break;
        }
        break;
    case ConnectorType::WallSocket:
        st = ChargerStatus::Charging;
        break;
    }

    static ChargerStatus LastStatus = ChargerStatus::SuspendedEVSE;
    if(LastStatus != st)
    {
        qWarning() << "CCHARGER: Deduce charge suspend status: " << ocpp::ChargerStatusStrs.at(st).data();
        LastStatus = st;
    }
    return st;
}

// Charge-Enabled (Command issued), but not yet in 'Charging'
ocpp::ChargerStatus CCharger::deduceBeforeChargeSuspendStatus()
{
    //Q_ASSERT(((ChargerNewRaw.State >> 16) & 0x0F) == 0x0); // 'Charging' == 0

    using namespace ocpp;
    ChargerStatus status = ChargerStatus::SuspendedEVSE;

    // Charge-Command issued, but not in Charging-State yet
    if (((ChargerNewRaw.State >> 28) & 0x0F) == 0x1) {
        switch ((ChargerNewRaw.State >> 24) & 0x0F) {
        case _OCPP_STATE_B:
            // Suspended because ChargingProfile limited max-current to 0.
            status = (appliedMaxCurrentSetting > 0)? ChargerStatus::SuspendedEV : ChargerStatus::SuspendedEVSE;
            break;
        default:
            break;
        }
    }

    static ChargerStatus LastStatus = ChargerStatus::SuspendedEVSE;
    if(LastStatus != status)
    {
        qWarning() << "CCHARGER: Deduce before charge suspend status: " << ocpp::ChargerStatusStrs.at(status).data();
        LastStatus = status;
    }
    return status;
}

bool CCharger::IsSuspendedStartEVSE()
{
    bool isSuspendedStart = false;

    // Charge-Command issued, but not in Charging-State yet
    if ((((ChargerNewRaw.State >> 28) & 0x0F) == 0x1)
        && (((ChargerNewRaw.State >> 16) & 0x0F) == 0x0)) {
        switch ((ChargerNewRaw.State >> 24) & 0x0F) {
        case _OCPP_STATE_B:
        case _OCPP_STATE_C: // Special case where the EV jump ahead to C, regardless of PWM; Simulated using DIP switch on our in-house charger-connector.
            // Suspended because ChargingProfile limited max-current to 0.
            if (appliedMaxCurrentSetting == 0)
                isSuspendedStart = true;
            break;
        default:
            break;
        }
    }

    static bool lastStatus = false;
    bool thisStatus = isSuspendedStart;
    if( thisStatus != lastStatus)
    {
        QString result = thisStatus? "true" : "false";
        qWarning() << "CCHARGER: is suspended start EVSE: " << result;
        lastStatus = thisStatus;
    }
    return thisStatus;
}

CCharger::Response CCharger::ChangeConnAvail(bool newAvailVal)
{
    Response changeAccepted = Response::Denied;

    if (newAvailVal == connAvailable) {
        qWarning() << "New Connector-Availability value is the same as old.";
        changeAccepted = Response::OK;
    }
    else {
        // To Available
        if (newAvailVal) {
            if (saveConnAvailabilityToFile(newAvailVal)) {
                connAvailable = true;
                emit connAvailabilityChanged(connAvailable);
                changeAccepted =  Response::OK;
            }
        }
        // To Unavailable
        else {
            // only accept if there is no ongoing transaction or booking
            switch (Status) {
            case ocpp::ChargerStatus::Available:
            case ocpp::ChargerStatus::Faulted:
                if (saveConnAvailabilityToFile(newAvailVal)) {
                    connAvailable = false;
                    emit connAvailabilityChanged(connAvailable);
                    changeAccepted =  Response::OK;
                }
                break;
            default:
                changeAccepted = Response::Busy;
                qWarning() << "Reject setting Connector to Unavailable while Charger-Status is "
                           << ocpp::ChargerStatusStrs.at(Status).data() << QString(" RawState: 0x%1").arg(ChargerNewRaw.State, 8, 16, QLatin1Char('0'));
                break;
            }
        }
    }

    qWarning() << "CCHARGER: Change connector availibility: " << static_cast<int>(changeAccepted);
    return changeAccepted;
}

EvChargingDetailMeterVal CCharger::GetMeterVal()
{
    qWarning() << "CCHARGER: Get meter Value";
    EvChargingDetailMeterVal meterVal;

    meterVal.meteredDt = QDateTime::currentDateTimeUtc();
    meterVal.Q = ChargedQ; // see fakeLoadEnergy
    meterVal.P = ChargerNewRaw.Power;
    if(charger::getMeterType() != charger::AcMeterPhase::SPM93)
    {
        meterVal.P *= 10;
    }
    meterVal.Pf = ChargerNewRaw.PFactor;
    meterVal.Freq = ChargerNewRaw.Frequency;
    meterVal.I[0] = ChargerNewRaw.Ia;
    meterVal.I[1] = ChargerNewRaw.Ib;
    meterVal.I[2] = ChargerNewRaw.Ic;
    meterVal.V[0] = ChargerNewRaw.Va;
    meterVal.V[1] = ChargerNewRaw.Vb;
    meterVal.V[2] = ChargerNewRaw.Vc;

    int CTRatio = charger::GetCTRatio();
    meterVal.I[0] *= CTRatio;
    meterVal.I[1] *= CTRatio;
    meterVal.I[2] *= CTRatio;

    return meterVal;
}

ocpp::ChargerStatus CCharger::StatusManage()
{
    using namespace ocpp;
    //qWarning() << "CCHARGER: Status manage";
    ulong prevState = State; // Save State before calling Refresh() in order to look for change.
    Refresh();  //刷新新的充电板信息
    ocpp::ChargerStatus newstatus =Status;
    ulong regtmp32 =(State>>16) &0x0F;

    if(stopChargeFlag){
        isSuspended = 0;
    }
    if(isSuspended == 1){
        return ocpp::ChargerStatus::SuspendedEVSE;
    }
    if(isSuspended == 2){
        return ocpp::ChargerStatus::SuspendedEV;
    }

    EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
    // state change from charging to no charge will cause transaction mess up
    if(stopChargeFlag){
        if ((deduceBeforeChargeSuspendStatus() == ChargerStatus::SuspendedEVSE)) {
            qWarning("Ccharger: Charger from SuspenedEVSE/SuspenedEV to available, return to finishing");
            ChargerIdle();
            regtmp32 = _C_STATE_FINISH;

            stopChargeFlag = false;
        }
        else if (((prevState >> 16) & 0x0F) == _C_STATE_CHARGEING && regtmp32 ==_C_STATE_NOCHARGE && !reg.getActiveTransactionId().isEmpty()) {
            qWarning("Ccharger: Charger from charging to available, return to finishing");
            regtmp32 = _C_STATE_FINISH;
        }
    }
    else{
        if (((prevState >> 16) & 0x0F) == _C_STATE_CHARGEING && regtmp32 ==_C_STATE_NOCHARGE && !reg.getActiveTransactionId().isEmpty()) {
            qWarning("Ccharger: Charger from charging to available, return to finishing");
            regtmp32 = _C_STATE_FINISH;
        }
        if ((regtmp32 == _C_STATE_FINISH) &&
                (((prevState >> 16) & 0x0F) == _C_STATE_CHARGEING)) {
            if(charger::GetStopTransactionOnEVSideDisconnect())
            {
                isSuspended = 2;
                return ocpp::ChargerStatus::SuspendedEV;
            }
        }
    }

    if(regtmp32 ==_C_STATE_NOCHARGE)
    {
        //if((Command &_C_CHARGECOMMAND) &&(CommandTimeout >120))
        if(((Command &_C_CHARGEMASK) ==_C_CHARGECOMMAND)) {
//            if(CommandTimeout >120)   //2017.06.13
//            { //有充电命令, 但命令超时
//                qWarning() << "Timeout waiting for CHARGING-STATE after Charging-Command issued.";
//                ocpp::ChargePointErrorCode boardError = extractErrorCodeFromState();
//                if(charger::isConnLockEnabled() && (IsUnLock()) && (boardError == ocpp::ChargePointErrorCode::NoError))
//                {
//                    errorCode = ocpp::ChargePointErrorCode::ConnectorLockFailure;
//                }
//                else
//                {
//                    UnLock();
//                    //EV迟迟未进入充电状态
//                    errorCode = boardError;
//                }

//                ChargerIdle();
//                CommandTimeout =0;
//                emit errorCodeChanged();
//                //保留原状态与状态串:Preparing 只有该状态才响应充电命令
//            }
//            else {
                // Charge-Command issued, but not yet in Charging-State
                newstatus = deduceBeforeChargeSuspendStatus();
                if (isResumedCharging) {
                    qDebug() << "Caught where default ErrorCode should reset";
                    errorCode = extractErrorCodeFromState(); // update the value
                }
//            }
        }
        //else if((Command &_C_CHARGEIDLE) && (CommandTimeout <120))  //&&(StatusChanged)
        else if(((Command &_C_CHARGEMASK) ==_C_CHARGEIDLE) && (CommandTimeout <120))    //2017.06.13
        { //取消充电, 主要是等待上一节idle命令的执行
            //等待状态与命令都已发送 Nothing to do.
            qDebug() <<"取消充电-idle!";
            if(IsIdleState()) {
                Command &=~_C_CHARGEMASK;  //已处于无充电命令状态, 发空命令
                applyDefaultCurrent();
            }
            else {
                // Charge-Command received by STM32 (ChargeEnable=1)
                // But Contactor still Open (Charging=0)
                qWarning() << "Here Here!";
            }
        }
        else if(IsFaultState())
        {
            newstatus =ocpp::ChargerStatus::Faulted;
            ocpp::ChargePointErrorCode boardError = extractErrorCodeFromState();
            if(errorCode != boardError)
            {
                errorCode = boardError;
                emit errorCodeChanged();
            }
            // disable the following line to develop without charger-board only
            if(Command &_C_CHARGECOMMAND) ChargerIdle();    //因还未开始充电, 取消充电
        }
        else if(bookingState == true)
        {
            errorCode = extractErrorCodeFromState();
            newstatus =ocpp::ChargerStatus::Reserved;
        }
        else if(IsPreparingState())
        {
            // Ignore Connected Cables if Connector-Unavailable
            if (connAvailable) {
                newstatus =ocpp::ChargerStatus::Preparing;
            }
            else {
                newstatus = ocpp::ChargerStatus::Unavailable;
            }
            errorCode = extractErrorCodeFromState();
        }
        else if (!(IsCablePlugIn() || IsEVConnected()))
        {
            if(pCurWin && ((pCurWin->windowTitle() =="ChargingW") || (pCurWin->windowTitle() =="FinishW")))
            {
                pCurWin->close();
                pCurWin = NULL;
            }
            if (connAvailable) {
                newstatus = ocpp::ChargerStatus::Available;
            }
            else {
                newstatus = ocpp::ChargerStatus::Unavailable;
            }
            errorCode = extractErrorCodeFromState();
        }
        else if(isIdleState(prevState) && IsIdleState())
        {
            if(!IsCablePlugIn() || !IsEVConnected()){
                newstatus = ocpp::ChargerStatus::Available;
            }
        }
    }
    else if(regtmp32 ==_C_STATE_CHARGEING)
    {
        if(IsChargingOrSuspended())
        {
            if((Command &(_C_CHARGECOMMAND |_C_CHARGEDT)) ==_C_CHARGESTOP)
            {
                if(CommandTimeout >120)
                {
                    //TODO: 停充超时 waiting...
                }
            }

            newstatus = deduceChargeSuspendStatus();
        }
        else if(Command &_C_CHARGECOMMAND)
        { //有充电命令
            if(StartDt.isNull())    //断电续充时不更新开始时间
            {
                if (IsResumedCharging())
                    qWarning() << "Warning: Start Time of Resumed-Charging is empty. Setting to Current Date/Time.";
                StartDt = QDateTime::currentDateTimeUtc();
                StopDt  = QDateTime();
            }
            newstatus = deduceChargeSuspendStatus();
        }
        else
        { //并没有充电命令, 但有充电状态返回: 取消充电
            ChargerIdle();
            CommandTimeout =0;
        }
    }
    else if(regtmp32 ==_C_STATE_FINISH)
    {
        newstatus =ocpp::ChargerStatus::Finishing;
        ocpp::ChargePointErrorCode boardError = extractErrorCodeFromState();
        if(boardError != errorCode)
        {
            errorCode = boardError;
            emit errorCodeChanged();
        }
        if(Status ==ocpp::ChargerStatus::Finishing)
        {
            //处理没弹出充电窗口就结束充电, 清除PCB状态:
            #if(1)
            if((pCurWin ==NULL) || ((pCurWin->windowTitle() !="ChargingW") && (pCurWin->windowTitle() !="FinishW")))
            {
                 EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
                if(charger::isStandaloneMode() || !reg.hasActiveTransaction())
                {
                    ChargerIdle();
                    qDebug() <<"充电结束, 没充电窗口 ccharger.cpp";
                }
            }
            #endif
        }
        else
        {
            if(StopReason == ocpp::Reason::Local)
            {
                if(IsUnnormalStop())
                {
                    if(State &_C_STATE_ESTOP)
                        StopReason = ocpp::Reason::EmergencyStop;
                    else //if(IsFaultState())
                        StopReason = ocpp::Reason::Other;
                }
                else if(((State >>20)&0x0F) ==3)
                {
                    StopReason = ocpp::Reason::EVDisconnected;
                }
                else if(((State >>20)&0x0F) ==4)
                {
                    // Frankie: Is this State-D?
                    StopReason = ocpp::Reason::Local;    //这不是标准值
                }
            }
            //POP Windows
            if(StopDt.isNull()) //记录停充时间
            {
                StopDt = QDateTime::currentDateTimeUtc();
                MeterStop = ChargedQ;
            }
        }
        if(charger::GetUnlockCarUnplug() && ((State >>20)&0x0F) ==3)
        {
            this->UnLock();
        }
        // Apply the Default Charging Profile at Charging-Finished
        applyDefaultCurrent();
        if (((prevState >> 16) & 0x0F) == _C_STATE_CHARGEING) {
            qWarning("FRANKIE: Charger went from Charging to Finish");
            emit chargeFinished(StopDt, MeterStop, StopReason);
        }
    }
    else if(regtmp32 ==_C_STATE_BOOKING)
    {
        newstatus =ocpp::ChargerStatus::Reserved;
        if(Status ==ocpp::ChargerStatus::Reserved)
        {
            ocpp::ChargePointErrorCode boardError = extractErrorCodeFromState();
            if(boardError != errorCode)
            {
                qWarning("Stop Current Transaction");
                emit chargeFinished(StopDt, MeterStop, StopReason);
            }
            else
            {
                qWarning("Transaction already stopped");
            }
        }
    }
    //else if(regtmp32 ==_C_STATE_BOOKING)
    // else if(bookingState == true)
    // {
    //     newstatus =ocpp::ChargerStatus::Reserved;
    //     if(Status ==ocpp::ChargerStatus::Reserved)
    //     {
    //         ocpp::ChargePointErrorCode boardError = extractErrorCodeFromState();
    //         if(boardError != errorCode)
    //         {
    //             errorCode = boardError;
    //             emit errorCodeChanged();
    //         }
    //     }
    // }

    // 由 Idle 状态转为  非闲置状态
    if (isIdleState(prevState) && !IsIdleState()) {
        qWarning("FRANKIE: Charge Command Acknowledged");
        if(connType != CCharger::ConnectorType::WallSocket)
            emit chargeCommandAccepted(StartDt, MeterStart, IsResumedCharging(), resumedTransactionId);
    }
    // 由 非Idle 状态转为 闲置状态
    if (!isIdleState(prevState) && IsIdleState()) {
        if (isChargeState(prevState))
            qWarning("FRANKIE: Charge went from charging/finish to idle");
        else {
            qWarning("FRANKIE: Charge went from non-charging/finish to idle");
            emit chargeCommandCanceled(QDateTime::currentDateTimeUtc(), MeterStart, StopReason);
        }
        if (connType == CCharger::ConnectorType::WallSocket) {
            // Wall-Socket does not know the Cable disconnected
            newstatus = ocpp::ChargerStatus::Available;
        }
    }

    return newstatus;
}

long CCharger::fakeLoadEnergy()
{
    //qWarning() << "CCHARGER: Fake load energy";
    static long chargedQ = 0;

    if (charger::enableFakeChargerLoad()) {
        const long offset = 99999000;
        //const long offset = 0;
        const long instantQ = 10;

        if (((State >>16) & 0x0F) ==_C_STATE_CHARGEING) {
            chargedQ += instantQ;
        }
        if (!IsChargeState()) {
            chargedQ = offset; // reset
        }
    }
    else {
        chargedQ = 0;
    }

    return chargedQ % 100000000;
}

void CCharger::setFakeCurrentOffset(long newOffset)
{
    //qWarning() << "CCHARGER: Set fake current offset";
    fakeLoadCurrent = newOffset;
}

long CCharger::getFakeCurrentOffset()
{
    //qWarning() << "CCHARGER: Get fake current offset";
    long currentOffset = 0;

    if (charger::enableFakeChargerLoad()) {

        if (((State >>16) & 0x0F) ==_C_STATE_CHARGEING) {
            currentOffset = fakeLoadCurrent;
        }
    }
    else {
        currentOffset = 0;
    }

    return currentOffset;
}

void CCharger::ChargerModBus2(const CHARGER_RAW_DATA &data)
{
    ChargerNewRaw = data;

    ChargerPostModBus();
}

void CCharger::ChargerModBus(const QByteArray &data)
{
    if(this->preRecvData != data){
        //qDebug() << "PCB State Changed: " << data.toHex();
        preRecvData = data;
    }
    //qWarning() << "CCHARGER: Charger Modbus";
    //[0]:addr [1]:fun [2][3]:mem [4][5]:w [6]:b [7]:rawdata 40
    qCDebug(ChargerMessages) << "CHARGER_RAW_DATA size: " << sizeof(CHARGER_RAW_DATA);
    qCDebug(ChargerMessages) << "ChargerData: (length=" << data.length() << ") " << data.toHex(' '); //test
    memcpy(&ChargerNewRaw, data.data()+7, sizeof(CHARGER_RAW_DATA));

    ChargerPostModBus();
}
void CCharger::ChargerPostModBus()
{
    appliedMaxCurrentSetting = ChargerNewRaw.MaxCurrent;
    ChargerNewRaw.Ia += getFakeCurrentOffset();
    if(!Initialized)
    {
        if(ReadRecord())
        {
            // TBD: Check whether it is EV connected before resume charge?
            Refresh();
            if (isEvConnected(ChargerNewRaw.State) && isCablePlugIn(ChargerNewRaw.CableMaxCurrent) && resumedTransactionId > 0 && !IsResumedCharging())
            {
                ResumeCharge();   //断电后继续充电
                qWarning("CCHARGER: ResumeCharging is called");
            }
            else
            {
                qWarning() << "CCHARGER: Unable to resume charging, not connected to EV or Cable not inserted or already resumed once";
                qWarning() << "CCHARGER: Send stop transaction";
                //EvChargingDetailMeterVal meter = GetMeterVal();
                emit stopResumeChargeTransaction(QString::number(resumedTransactionId), StopDt, MeterStop, ocpp::Reason::Local, IdTag);
                isResumedCharging = false;
            }
            ClearRecord();   //断电续充只执行一次
            qWarning() << "CCHARGER: Clear record";
            qDebug()<<RemainTime <<" ######### "<<StartDt.toString(Qt::ISODateWithMs);
            //qDebug()<<" type:"<<m_ChargerA.SocketType;
            //qDebug()<<" id:"<<m_ChargerA.IdTag;
        }

        ReadReserveRecord();
        qWarning() << "CCHARGER: start up read reserve record";

        qWarning() << "Charger-Board Firmware Version: " << ChargerNewRaw.FirmwareVer;
    }
    Initialized =true;
}

void CCharger::Refresh()
{
    //qWarning() << "CCHARGER: Refresh";
#if(_NO_PCB_TEST)      //0 normal run
    SoftPCB();
    ChargerNewRaw.cableMaxCurrent =0;   //No canble
    ChargerNewRaw.State =0xE083;  //重载使State运算处理后=0无故障 test!
#endif
    State       =ChargerNewRaw.State ^0xE083;   //改为正逻辑
    State       &=0xFFFFFF83;                   //清0不用的位
    ChargedQ    =ChargerNewRaw.Q + fakeLoadEnergy();

    if(IsChargeState())
    {
        RemainTime  = ChargerNewRaw.Time;
    }

    cableMaxCurrent = ChargerNewRaw.CableMaxCurrent;
    qCDebug(ChargerMessages) << QString("Raw State: %1").arg(ChargerNewRaw.State, 8, 16, QLatin1Char('0'));
    //qDebug() << QString("Adjusted State: %1").arg(State, 8, 16, QLatin1Char('0'));
    //qDebug() <<QString("state: %1 Q:%2 %3, %4").arg(State,32,2,QLatin1Char('0')).arg(ChargedQ/10.0).arg(cableMaxCurrent).arg(RemainTime);
    //qDebug() <<"va:"<<ChargerNewRaw.Va/100.0<<" vb:"<<ChargerNewRaw.Vb/100.0<<" vc:"<<ChargerNewRaw.Vc/100.0;
    //qDebug() <<"Ia:"<<ChargerNewRaw.Ia/1000.0<<" Ib:"<<ChargerNewRaw.Ib/1000.0<<" Ic:"<<ChargerNewRaw.Ic/1000.0;
    //qDebug() <<"Power:" <<ChargerNewRaw.Power/100.0 <<"PF:" <<ChargerNewRaw.PFactor/1000.0;
}

bool CCharger::NewRecord()
{
    qWarning() << "CCHARGER: New record";
    return ModifyRecord();
}

bool CCharger::ModifyRecord()
{
    qWarning() << "CCHARGER: Modify record";
    bool retCode =false;
    CHARGE_RECORD_TYPE record;
    QString Filepath;

    if(chrgProgressCsvPathfilecontrol)
    {
        Filepath = chrgProgressCsvPath;
        chrgProgressCsvPathfilecontrol = false;
    }
    else
    {
        Filepath = chrgProgressCsvPathBackup;
        chrgProgressCsvPathfilecontrol = true;
    }
    qWarning() << Filepath;
    QFile recordfile(Filepath);
    if(recordfile.open(QFile::WriteOnly |QIODevice::Unbuffered))    // |QIODevice::Truncate
    {
        EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
        record.ConnectorId  =1;
        int transId = -1; // default for standalone-mode
        if (!charger::isStandaloneMode() && !reg.getActiveTransactionId().isEmpty())
            transId = reg.getActiveTransactionId().toInt();
        qDebug() << "Writing TransactionId = " << transId;
        record.TransactionId = transId;
        record.Time          = ChargeTimeValue;
        record.MeterStartVal = MeterStart;
        record.MeterStopVal = ChargedQ;
        record.Q            = ChargeQValue;
        record.RemainTime   = RemainTime;
        record.RemainQ      = ChargedQ;
        record.lastChargingProfile = profileCurrent;
        record.Status       = static_cast<u8>(ocpp::ChargerStatus::Charging);
        bool result = isResumedCharging;
        record.isResumedCharge = result;
        QString localTimeStr = StartDt.toLocalTime().toString(Qt::DateFormat::ISODateWithMs);
        memcpy(record.StartDt, localTimeStr.toLocal8Bit().data(), 24);
        QString NowDt = QDateTime::currentDateTime().toLocalTime().toString(Qt::DateFormat::ISODateWithMs);
        memcpy(record.StopDt, NowDt.toLocal8Bit().data(), 24);
        record.SocketType   = static_cast<uchar>(connType);
        record.IdLength = IdTag.length();
        if (record.IdLength > CHARGE_RECORD_TYPE::MaxIdLength) {
            qWarning("ModifyRecord(): Id Length truncated.");
            record.IdLength = CHARGE_RECORD_TYPE::MaxIdLength;
        }
        memcpy(record.ID, IdTag.toLocal8Bit().data(), record.IdLength);
        memcpy(record.Pwd, Password.toLocal8Bit().data(), Password.length());   //<=4
        //record.StartDt[24] ='\0';

        recordfile.write((char *)&record, sizeof(CHARGE_RECORD_TYPE));

        retCode =true;
        recordfile.close();
    }
    else
    {
        qWarning("cannot open recordfile to write");
    }

    return retCode;
}

bool CCharger::ReadRecord()
{
    qWarning() << "CCHARGER: Read record";
    bool retCode =false;
    CHARGE_RECORD_TYPE record;
    QDateTime recorddt;
    QDateTime record2dt;
    QString Filepath;

    bool recordok = false;
    bool record2ok = false;

    QFile record1file(chrgProgressCsvPath);
    if(record1file.open(QIODevice::ReadOnly))
    {
        if(record1file.read((char *)&record, sizeof(CHARGE_RECORD_TYPE)) ==sizeof(CHARGE_RECORD_TYPE))
        {
            recordok = true;
            recorddt = QDateTime::fromString(QString(record.StopDt), Qt::DateFormat::ISODateWithMs);
        }
    }
    record1file.close();

    QFile record2file(chrgProgressCsvPathBackup);
    if(record2file.open(QIODevice::ReadOnly))
    {
        if(record2file.read((char *)&record, sizeof(CHARGE_RECORD_TYPE)) ==sizeof(CHARGE_RECORD_TYPE))
        {
            record2ok = true;
            record2dt = QDateTime::fromString(QString(record.StopDt), Qt::DateFormat::ISODateWithMs);
        }
    }
    record2file.close();

    if(recordok)
    {
        if(record2ok)
        {
            if(recorddt > record2dt)
            {
                Filepath = chrgProgressCsvPath;
                qWarning() << "CCHARGER: Both record ok, reading record";
            }
            else
            {
                Filepath = chrgProgressCsvPathBackup;
                qWarning() << "CCHARGER: Both record ok, reading recordbackup";
            }
        }
        else
        {
            Filepath = chrgProgressCsvPath;
            qWarning() << "CCHARGER: record2 not ok, reading record";
        }
    }
    else if(record2ok)
    {
        Filepath = chrgProgressCsvPathBackup;
        qWarning() << "CCHARGER: record not ok, reading record2";
    }


    QFile recordfile(Filepath);
    if(recordfile.open(QIODevice::ReadOnly))
    {
        if(recordfile.read((char *)&record, sizeof(CHARGE_RECORD_TYPE)) ==sizeof(CHARGE_RECORD_TYPE))
        {
            if((record.Status == static_cast<u8>(ocpp::ChargerStatus::Charging)) &&(record.RemainTime >0))
            {
                ChargeTimeValue =record.Time;
                MeterStart = record.MeterStartVal;
                MeterStop = record.MeterStopVal;
                RemainTime      =record.RemainTime;
                ChargeQValue    =record.Q;
                ChargedQ        =record.RemainQ;
                resumedTransactionId = record.TransactionId;
                profileCurrent = record.lastChargingProfile;
                record.StartDt[23] ='\0';
                StartDt         = QDateTime::fromString(QString(record.StartDt), Qt::DateFormat::ISODateWithMs);
                StopDt          = QDateTime::fromString(QString(record.StopDt), Qt::DateFormat::ISODateWithMs);
                connType        = static_cast<ConnectorType>(record.SocketType);
                IdTag           = QString::fromLocal8Bit(record.ID, record.IdLength);
                record.Pwd[4]    ='\0';
                Password         =QString(record.Pwd);
                isResumedCharging = record.isResumedCharge;

                retCode =true;
                qWarning()<<record.TransactionId;
            }
            else
            {
                qWarning("CCHARGER: no match recordfile data");
            }
        }
        else
        {
            qWarning("CCHARGER: recordfile size dont match");
        }
        qWarning("CCHARGER: ReadRecod():");
        qWarning() << "CCHARGER: " << StopDt;

        recordfile.close();
    }
    else
    {
        qWarning("CCHARGER: cannot open recordfile to read");
    }

    return retCode;
}

void CCharger::ClearRecord()
{
    qWarning() << "CCHARGER: Clear record";
    QFile recordfile(chrgProgressCsvPath);
    recordfile.remove();
    QFile record1file(chrgProgressCsvPathBackup);
    record1file.remove();
}

void CCharger::SendUdpLog(const QByteArray &data)
{
    //qWarning() << "CCHARGER: SendUdpLog";
    Q_UNUSED(data);
    /* Frankie: comment this out until we find out what this log-server is
    QUdpSocket UdpLog;

    UdpLog.writeDatagram(data, QHostAddress::LocalHost, 5015);
    */

}

bool CCharger::IsBookingState()
{
    // static bool lastStatus = false;
    // bool thisStatus = ((State &_C_STATE_CHARGEMASK) ==(_C_STATE_BOOKING<<16));
    // if( thisStatus != lastStatus)
    // {
    //     QString result = thisStatus? "true" : "false";
    //     qWarning() << "CCHARGER: is booking state: " << result;
    //     lastStatus = thisStatus;
    // }
    // return thisStatus;
    return bookingState;
}

void CCharger::SetBooking()
{
    qWarning() << "CCHARGER: Set booking state";
    bookingState = true;
    CreateReserveRecord();
    // Command =_C_BOOKINGCOMMAND;
    // CommandTimeout =0;
}

void CCharger::CancelBooking()
{
    qWarning() << "CCHARGER: Cancel booking state";
    m_ChargerA.ReservationId = NULL;
    bookingState = false;
}


bool CCharger::saveConnAvailabilityToFile(bool newCnnAvlVal)
{
    qWarning() << "CCHARGER: Save connector availibility to file";
    bool updated = false;
    try {
        if (newCnnAvlVal) {
            // remove file
            if (QFile::exists(connAvailSettingPath)) {
                if (!QFile::remove(connAvailSettingPath)) {
                    throw std::runtime_error("Unable to remove Connector Settings");
                }
            }
        }
        else {
            // create file with timestamp inside
            QFile file(connAvailSettingPath);
            if (file.open(QIODevice::Truncate|QIODevice::ReadWrite|QIODevice::Text)) {
                QTextStream record(&file);
                record << QDateTime::currentDateTime().toString(Qt::ISODate);
                if (file.error() != QFile::FileError::NoError)
                    throw new std::runtime_error(file.errorString().toStdString());
                file.close();
            }
            else {
                throw new std::runtime_error(file.errorString().toStdString());
            }
        }
        updated = true;
    }
    catch (std::exception &e)
    {
        updated = false;
        qWarning() << "Update Connector-Availability settings file failed." << e.what();
    }

    return updated;
}

void CCharger::loadConnAvailabilityFromFile()
{
    qWarning() << "CCHARGER: Load connector availibility from file";
    QFile settingsFile(connAvailSettingPath);
    if (settingsFile.exists()) {
        qWarning() << connAvailSettingPath << " Exists. Connector Availability set to false";
        connAvailable = false;
    }
    else {
        connAvailable = true;
    }
}

void CCharger::SaveCurrentChargeRecord(TransactionRecordFile &recordFile)  // 在ocpp/finishw中调用
{
    qWarning() << "CCHARGER: Save current charge record";
    EvChargingDetailMeterVal startVal;
    EvChargingDetailMeterVal endVal;

    startVal.meteredDt = StartDt;
    endVal.meteredDt = StopDt;

    startVal.Q = MeterStart;
    endVal.Q = MeterStop;

    recordFile.addEntry(QString(), IdTag, startVal, endVal);
}

void CCharger::SavePowerMeterRecord(PowerMeterRecFile &recordFile)
{
    qWarning() << "CCHARGER: Save power meter record";
    EvChargingDetailMeterVal meterVal;

    meterVal.Q = ChargerNewRaw.Q;
    meterVal.P = ChargerNewRaw.Power;
    meterVal.Pf = ChargerNewRaw.PFactor;
    meterVal.I[0] = ChargerNewRaw.Ia;
    meterVal.I[1] = ChargerNewRaw.Ib;
    meterVal.I[2] = ChargerNewRaw.Ic;
    meterVal.V[0] = ChargerNewRaw.Va;
    meterVal.V[1] = ChargerNewRaw.Vb;
    meterVal.V[2] = ChargerNewRaw.Vc;
    meterVal.Freq = ChargerNewRaw.Frequency;

    if(charger::getMeterType() != charger::AcMeterPhase::SPM93)
    {
        meterVal.P *= 10;
    }
    int CTRatio = charger::GetCTRatio();
    meterVal.I[0] *= CTRatio;
    meterVal.I[1] *= CTRatio;
    meterVal.I[2] *= CTRatio;

    if (!recordFile.addEntry(QDateTime::currentDateTime(), meterVal)) {
        qWarning() << "Error saving meter value to file.";
    }
}

void CCharger::SaveMsgRecord(QString newmsg)
{
    qWarning() << "CCHARGER: Save message record";
    QFile file("/home/pi/Downloads/MsgRecord.csv");
    if(OldMsg !=newmsg)
    {
        if(file.size() > maxNumMsgRecords) RemoveOldRecords(file, minNumMsgRecordsToKeep);
        if(file.open(QIODevice::Append|QIODevice::WriteOnly|QIODevice::Text))
        {
            QTextStream record(&file);
            if(file.size() ==0)
            {
                //record <<QObject::trUtf8("TransactionID, 故障時間, 故障碼, 詳情\n");
                record <<QObject::tr("TransactionID, Fault Time, Fault Code, Details\n");
            }

            //{{
            ulong   error;
            QString details("");
            if(newmsg =="E-4083")
                details ="PCB";
            else
            {
                error =newmsg.mid(2).toULong();
                if(error &_C_STATE_ESTOP) details +="STOP";
                if(error &_C_STATE_EARTH) details +=" EARTH";
                if(error &_C_STATE_DIODE) details +=" DIODE";
                if(error &_C_STATE_CONTACTOR) details +=" CON";
                if(error &_C_STATE_VOLTAGE) details +=" VOLTAGE";
                if(error &_C_STATE_CURRENT) details +=" CURRENT";
                if(error &0x0080) details +=" MCB";
                if(error &0x0002) details +=" SURGE";
                if(error &0x0001) details +=" RCCB";
            }
            //}}
            EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
            int transId = charger::isStandaloneMode()? -1 : reg.getActiveTransactionId().toInt();
            record <<QString("%1, %2, %3, %4\n").arg(transId).arg(QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss")).arg(newmsg).arg(details);
            file.close();

            OldMsg =newmsg;
        }
    }
}

void CCharger::StartChargeToFull()
{
    qWarning() << "CCHARGER: Start charge to full";
    ChargeTimeValue = ChargeToFullTimeSecs;  //100h
    RemainTime = ChargeTimeValue;
    StartCharge();
}

bool CCharger::IsChargingToFull()
{
    QString result = (ChargeTimeValue == ChargeToFullTimeSecs)? "true" : "false";
    qWarning() << "CCHARGER: is charging to full: " << result;
    return (ChargeTimeValue == ChargeToFullTimeSecs);
}

void CCharger::CreateReserveRecord()
{
    qWarning() << "CCHARGER: CreateReserveRecord resid:" << m_ChargerA.ReservationId << " idtag: " << m_ChargerA.ResIdTag << " ExpiryDate: " << m_ChargerA.ExpiryDate;
    RESERVE_RECORD_TYPE record;
    QFile recordfile(reserveRecordPath);
    if(recordfile.open(QFile::WriteOnly |QIODevice::Unbuffered))    // |QIODevice::Truncate
    {
        record.ReservationId = m_ChargerA.ReservationId;
        record.IdLength = m_ChargerA.ResIdTag.length();
        if (record.IdLength > CHARGE_RECORD_TYPE::MaxIdLength) {
            qWarning("ModifyRecord(): Id Length truncated.");
            record.IdLength = CHARGE_RECORD_TYPE::MaxIdLength;
        }
        memcpy(record.ID, m_ChargerA.ResIdTag.toLocal8Bit().data(), record.IdLength);

        QString localTimeStr = m_ChargerA.ExpiryDate.toLocalTime().toString(Qt::DateFormat::ISODateWithMs);
        memcpy(record.ExpiryDate, localTimeStr.toLocal8Bit().data(), 24);
        recordfile.write((char *)&record, sizeof(RESERVE_RECORD_TYPE));
        recordfile.close();
    }
}

void CCharger::ReadReserveRecord()
{
    qWarning() << "CCHARGER: ReadReserveRecord";
    RESERVE_RECORD_TYPE record;
    QFile recordfile(reserveRecordPath);
    if(recordfile.open(QIODevice::ReadOnly))
    {
        if(recordfile.read((char *)&record, sizeof(RESERVE_RECORD_TYPE)) ==sizeof(RESERVE_RECORD_TYPE))
        {
            m_ChargerA.ReservationId = record.ReservationId;
            m_ChargerA.ResIdTag = QString::fromLocal8Bit(record.ID, record.IdLength);;
            m_ChargerA.ExpiryDate = QDateTime::fromString(QString(record.ExpiryDate), Qt::DateFormat::ISODateWithMs);;
            SetBooking();
        }
    }
    recordfile.close();
    qWarning() << "CCHARGER: ReadReserveRecord resid:" << m_ChargerA.ReservationId << " idtag: " << m_ChargerA.ResIdTag << " ExpiryDate: " << m_ChargerA.ExpiryDate;
}

void CCharger::DeleteReserveRecord()
{
    qWarning() << "CCHARGER: DeleteReserveRecord";
    QFile recordfile(reserveRecordPath);
    recordfile.remove();
}

void CCharger::onQueueingCountRemainingTime()
{
    qWarning() << "CCHARGER: onQueueingCountRemainingTime";
    StopCharge();
}

void CCharger::accumChargedQBeforeSusp(u32 bf)
{
    this->accumulatedChargedQ = this->accumulatedChargedQ + bf;
}

u32 CCharger::getTotalChargedQ()
{
    return this->ChargedQ + this->accumulatedChargedQ;
}


#if false
void CCharger::SoftPCB()
{
    static int com_delay =0;
    static int min=1;

    ChargerNewRaw.cableMaxCurrent =32;
    ChargerNewRaw.Q =10;
    ChargerNewRaw.Ia =ChargerNewRaw.Ib =ChargerNewRaw.Ic =0;
    ChargerNewRaw.Va =ChargerNewRaw.Vb =ChargerNewRaw.Vc =220;
    ChargerNewRaw.State &=0xFFFFE083;
    ChargerNewRaw.State |=0x4083;

    if(Command &_C_CHARGECOMMAND)
    { //charge:
        if(IsCablePlugIn() &&IsEVConnected())
        {
            if(((ChargerNewRaw.State >>16) &0xf) !=_C_STATE_CHARGEING)
            {
                if(((ChargerNewRaw.State >>16) &0xf) !=_C_STATE_FINISH)
                if(com_delay++ >6)
                {
                    com_delay =0;
                    ChargerNewRaw.State =0x13014083;
                    if(ChargeTimeValue ==0) ChargeTimeValue =90;
                    ChargerNewRaw.Time =ChargeTimeValue;

                    min =2;
                    ChargedQ =0;
                    MeterStop =ChargedQ;
                }
            }
            else
            {
                min--;
                if(!min)
                {
                    min =2;
                    ChargedQ++;
                    MeterStop =ChargedQ;
                    if(ChargerNewRaw.Time)
                    {
                        ChargerNewRaw.Time--;
                    }
                    else
                    {
                        ChargerNewRaw.State =(ChargerNewRaw.State &0xfff0ffff) |0x22000;
                        qDebug() <<"计时到!";
                    }
                }
            }
        }
    }
    else if((Command &(_C_LOCKCOMMAND|_C_LOCKCTRL)) ==(_C_LOCKCOMMAND|_C_LOCKCTRL))
    { //lock:
        if(IsCablePlugIn() &&IsUnLock())
        {
            if(com_delay++ >6)
            {
                com_delay =0;
                ChargerNewRaw.State &=~(u32)_C_STATE_LOCK;
            }
        }
    }
    else if((Command &(_C_LOCKCOMMAND|_C_LOCKCTRL)) ==_C_LOCKCTRL)
    { //unlock:
        if(!IsUnLock())
        {
            if(com_delay++ >6)
            {
                com_delay =0;
                ChargerNewRaw.State |=_C_STATE_LOCK;
                if((ChargerNewRaw.State &0x000f0000) ==0x00010000)  //充电则充电结束
                    ChargerNewRaw.State =(ChargerNewRaw.State &0xfff0ffff) |0x20000;
            }
        }
    }
    else if((Command &0x1c) ==_C_CHARGESTOP)
    { //stop:
        if((ChargerNewRaw.State &0x000f0000) ==0x00010000)  //充电则充电结束
        {
            if(com_delay++ >6)
            {
                com_delay =0;
                ChargerNewRaw.State =(ChargerNewRaw.State &0xfff0ffff) |0x22000;
            }
        }
    }
    else if((Command &0x1c) ==_C_CHARGEIDLE)
    {
        if(((State >>16) &0xf) !=_C_STATE_NOCHARGE)
        {
            if(com_delay++ >6)
            {
                com_delay =0;
                //ChargerNewRaw.State =0x0200E083;
                ChargerNewRaw.State =0x0000E083;
                //ChargerNewRaw.cableMaxCurrent =0;
                MaxCurrentValue =cableMaxCurrent;
            }
        }
    }

    if(((State >>16) &0xf) ==_C_STATE_NOCHARGE)
    {
        if(com_delay++ >30)
        {
            ChargerNewRaw.State =0x0200E083;    //默认CP,PP线状态改动, 控制13A/TYPE 2:
        }
        else //if(com_delay >20)
        {
            ChargerNewRaw.cableMaxCurrent =0;
        }
    }
    //todo

    //---------------------
    //{{ChargerModBus():
    if(!Initialized)
    {
        if(ReadRecord())
        {
            StartCharge();   //断电后继续充电
            ClearRecord();   //断电续充只执行一次
            qDebug() <<RemainTime <<" ######### "<<StartDt;
        }
    }
    Initialized =true;
    //}}
    //qDebug() <<QString("RAW IS [%1]").arg(ChargerNewRaw.State, 8, 16, QLatin1Char('0')).toUpper();
}
#endif
