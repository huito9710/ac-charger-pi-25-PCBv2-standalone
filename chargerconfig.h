#ifndef CHARGERCONFIG_H
#define CHARGERCONFIG_H

#define VERSION "v2.25.1_standalone"

#include <string>
#include <QString>

namespace charger {

    enum class DisplayLang : int
    {
        SimplifiedChinese = 0,
        Chinese = 1,
        English = 2,
    };

    enum class ChargeMode
    {
        ChargeToFull,
        UserSelect,
        NoAuthChargeToFull,
    };

    enum class PayMethod {
        App,
        Octopus,
        RFID,
        App_Octopus,
    };

    // Important: the integer value is what the STM32 charger-board
    // expects from here.
    enum class AcMeterPhase : int {
        SPM93 = 0,
        SPM91 = 1
    };

    // Developer value-overrides
    bool ignoreThisVendorErrorCode(const std::string &errorcode);
    bool overrideChargeTimeValue();
    unsigned long getOverrideChargeTimeValue();
    bool enableFakeChargerLoad();

    void initChargerConfig();
    bool isStandaloneMode();
    bool allowOfflineCharging(); // allow use of RFID to start charging
    bool useSafeModeCurrentWhenOffline(); // use Safe-Mode Current during offline charging
    PayMethod getPayMethod();
    ChargeMode getChargingMode();
    bool getWallSocketSupported();
    bool getAC63ANoLockSupported();
    bool isConnLockEnabled();
    bool getRfidSupported();
    QString getOcppCsUrl();
    std::string getChargePointSerialNum();
    std::string getChargerNum();
    std::string getChargerName();

    bool forceOcppMeterValueKWh();
    AcMeterPhase getMeterType();
    int getMeterPhase();
    int getSafeModeCurrent();
    int getMaxCutoffChargeCurrent();
    int getMinCutoffChargeCurrent();
    int getCutoffDebouceTime();
    bool reduceScreenTransitions();
    DisplayLang getCurrDisplayLang();
    DisplayLang toggleDisplayLang();
    void setCurrDisplayLang(DisplayLang lang);
    bool isHardKeySupportRequired();

    int getAuthorizationCacheMaxSize();

    QString getSoftwareVersion();
    bool passwordAuthEnabled();

    // Run-time values
    ChargeMode getCurrentChargingMode();

    //show load management message or not
    bool GetLoadManagementShow();
    int  GetChargeTimeShow();
    bool GetReaminTimeShow();
    bool GetVoltageShow();
    bool GetCurrentShow();
    bool GetEnergyShow();
    bool GetSkipPayment();
    bool GetSinglePhaseDisplay();
    int GetCTRatio();
    ulong * GetTimeSelection();
    bool GetQueueingMode();
    bool GetAuthStop();
    bool GetLowCurrentStop();
    bool GetNoChargeToFull();
    bool GetUnlockCarUnplug();
    int GetDistThreshold();
    int GetDistSenseTimes();
    bool GetKeepLockedWhenSuspended();
    bool NeedRFIDCheckBeforeSuspend();
    bool GetStopTransactionOnEVSideDisconnect();
    bool GetEnablePauseCharging();
}


#endif // CHARGERCONFIG_H

