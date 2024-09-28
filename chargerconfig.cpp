#include <QString>
#include <QTranslator>
#include <QDebug>
#include <QApplication>

#include "gvar.h"
#include "api_function.h"
#include "chargerconfig.h"
#include "hostplatform.h"

namespace charger {

    // INI Configuration
    struct ChargerConfiguration
    {
    public:
        bool standaloneMode; // true: standalone, false: online/ocpp-mode
        bool offlineChargingSupported;
        enum ChargeMode chargeMode;
        CCharger::ConnectorType cableConnType; // only presents the value stored in INI file, not current user-selection.
        bool rfidSupported;
        PayMethod pay;
        bool skipPayment;
        QString ocppCsUrlStr;
        std::string chargerSerNum;
        std::string chargerNum;
        std::string chargerName;
        bool ocppMeterUnitKWh;
        AcMeterPhase meterPhase;
        bool singlePhaseDisplay;
        int CTRatio;
        bool useSafeModeCurrent;
        int safeModeCurrent; //离线时安全模式的安全电流
        int minChargeCurrent;
        int maxChargeCurrent;
        int minChargeDebouceTime;
        bool hasDisplayUnit;
        DisplayLang lang;
        QTranslator translator;
        bool hardwareKeySupport;
        bool showloadmanagement;//show load management msg?
        int  showChargingTime;
        bool showRemainTime;
        bool showVoltage;
        bool showCurrent;
        bool showEnergy;
        ulong TimeSelection[4];
        bool queueingMode;
        bool authStop;
        bool lowCurrentStop;
        bool noChargeToFull;
        bool unlockCarUnplug;
        int  distThreshold;
        int  distSenseTimes;
        bool keepLockedWhenSuspended;
        bool needRFIDCheckBeforeSuspend;
        bool stopTransactionOnEVSideDisconnect;
        bool enablePauseCharging;

    public:
        static ChargerConfiguration &instance() {
            static ChargerConfiguration thisCfg;
            return thisCfg;
        }

    private:
        ChargerConfiguration()
        {
            bool translationLoaded = translator.load(QString("%1ChargerV2_zh_HK").arg(charger::Platform::instance().translationFileDir));
            qDebug() << std::ios::boolalpha << "Translation Loaded: " << translationLoaded;
            lang = DisplayLang::Chinese; // Default to Chinese


            QString itemVal;

            standaloneMode = false;
            GetSetting(QStringLiteral("[NETVER]"), itemVal);
            standaloneMode = (itemVal.toInt() == 0);

            offlineChargingSupported = false;
            GetSetting(QStringLiteral("[OfflineCharging]"), itemVal);
            offlineChargingSupported = (itemVal.toInt() != 0);

            itemVal = QString();
            GetSetting(QStringLiteral("[CHARGINGMODE]"), itemVal);
            switch (itemVal.toInt()) {
            case 0: chargeMode = ChargeMode::ChargeToFull; break;
            case 1: chargeMode = ChargeMode::UserSelect; break;
            case 2: chargeMode = ChargeMode::NoAuthChargeToFull; break;
            default:
                throw "Unexpected CHARGINGMODE value from settings.ini";
            }

            itemVal = QString();
            GetSetting(QStringLiteral("[SOCKET]"), itemVal);
            switch (itemVal.toInt()) {
            case 0: cableConnType = CCharger::ConnectorType::EvConnector; break;
            case 1: cableConnType = CCharger::ConnectorType::WallSocket; break;
            case 2: cableConnType = CCharger::ConnectorType::AC63A_NoLock; break;
            default:
                throw "Unexpected SOCKET value from settings.ini";
            }

            itemVal = QString();
            GetSetting(QStringLiteral("[RFID]"), itemVal);
            rfidSupported = (itemVal =="1");

            if (rfidSupported) {
                pay = PayMethod::RFID;
            }
            else {
                bool byApp = false;
                bool byOctopus = false;

                itemVal = QString();
                GetSetting(QStringLiteral("[APP]"), itemVal);
                if(itemVal =="1")
                    byApp = true;
                itemVal = QString();
                GetSetting(QStringLiteral("[Octopus]"), itemVal);
                if(itemVal =="1")
                    byOctopus = true;

                if (byApp && byOctopus)
                    pay = PayMethod::App_Octopus;
                else if (byApp)
                    pay = PayMethod::App;
                else if (byOctopus)
                    pay = PayMethod::Octopus;
//                else
//                    throw "Unhanded payment/authorization settings.";
            }

            itemVal = QString();
            if(GetSetting(QStringLiteral("[No_Pwd]"), itemVal))
                skipPayment = (itemVal.toInt() == 1);
            else
                skipPayment = false;

            itemVal = QString();
            GetSetting(QStringLiteral("[HOST&URL]"), itemVal);
            ocppCsUrlStr = itemVal;

            itemVal = QString();
            GetSetting(QStringLiteral("[CPSERIALNO]"), itemVal);
            chargerSerNum = itemVal.toStdString();

            itemVal = QString();
            GetSetting(QStringLiteral("[CHARGER NO]"), itemVal);
            chargerNum = itemVal.toStdString();

            itemVal = QString();
            GetSetting(QStringLiteral("[chargerName]"), itemVal);
            chargerName = itemVal.toStdString();

            itemVal = QString();
            GetSetting(QStringLiteral("[Kwh]"), itemVal);
            ocppMeterUnitKWh = (itemVal.compare("1") == 0)? true : false;

            itemVal = QString("3"); // default is SPM93
            GetSetting(QStringLiteral("[PHASE]"), itemVal);
            if (itemVal.compare("3") == 0)
                meterPhase = AcMeterPhase::SPM93;
            else
                meterPhase = AcMeterPhase::SPM91;

            useSafeModeCurrent = false;
            GetSetting(QStringLiteral("[safeMode]"), itemVal);
            useSafeModeCurrent = (itemVal.toInt() != 0);

            singlePhaseDisplay = false;
            GetSetting(QStringLiteral("[singlePhaseDisplay]"), itemVal);
            singlePhaseDisplay = (itemVal.toInt() != 0);

            CTRatio = 1;
            GetSetting(QStringLiteral("[CTRatio]"), itemVal);
            CTRatio = itemVal.toInt();

            itemVal = QString();
            if(GetSetting(QStringLiteral("[safeCurrent]"), itemVal))
                safeModeCurrent = itemVal.toInt();
            else
                safeModeCurrent = 16; // default

            itemVal = QString();
            if(GetSetting(QStringLiteral("[MAX CURRENT]"), itemVal))
                maxChargeCurrent = itemVal.toInt();
            else
                maxChargeCurrent = 255; // default

            itemVal = QString();
            if(GetSetting(QStringLiteral("[CurrentStop]"), itemVal))
                minChargeCurrent = itemVal.toInt();
            else
                minChargeCurrent = 0; // default

            itemVal = QString();
            if(GetSetting(QStringLiteral("[TimeStop]"), itemVal))
                minChargeDebouceTime = itemVal.toInt()/2;
            else
                minChargeDebouceTime = 0; // default

            itemVal = QString();
            if(GetSetting(QStringLiteral("[NoScreen]"), itemVal))
                hasDisplayUnit = (itemVal.toInt() == 0);
            else
                hasDisplayUnit = true; // Default

            //show load management?
            itemVal = QString();
            if(GetSetting(QStringLiteral("[LoadManagementMsg]"), itemVal))
                showloadmanagement = (itemVal.toInt() == 1);
            else
                showloadmanagement = false;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[ChargingTimeMsg]"), itemVal))
                showChargingTime = itemVal.toInt();
            else
                showChargingTime = 1;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[RemainTimeMsg]"), itemVal))
                showRemainTime = (itemVal.toInt() == 1);
            else
                showRemainTime = true;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[DistanceThreshold]"), itemVal))
                distThreshold = itemVal.toInt();
            else
                distThreshold = 100;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[DistanceSenseTimes]"), itemVal))
                distSenseTimes = itemVal.toInt();
            else
                distSenseTimes = 10;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[LOCKSUSP]"), itemVal))
                keepLockedWhenSuspended = (itemVal.toInt() == 1 );
            else
                keepLockedWhenSuspended = false;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[NEEDRFIDCkBfSUSP]"), itemVal))
                needRFIDCheckBeforeSuspend = (itemVal.toInt() == 1 );
            else
                needRFIDCheckBeforeSuspend = false;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[StopTransactionOnEVSideDisconnect]"), itemVal))
                stopTransactionOnEVSideDisconnect = (itemVal.toInt() == 1 );
            else
                stopTransactionOnEVSideDisconnect = true;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[EnablePauseCharging]"), itemVal))
                enablePauseCharging = (itemVal.toInt() == 1 );
            else
                enablePauseCharging = true;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[VoltageMsg]"), itemVal))
                showVoltage = (itemVal.toInt() == 1);
            else
                showVoltage = true;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[CurrentTimeMsg]"), itemVal))
                showCurrent = (itemVal.toInt() == 1);
            else
                showCurrent = true;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[EnergyTimeMsg]"), itemVal))
                showEnergy = (itemVal.toInt() == 1);
            else
                showEnergy = true;

            itemVal = QString();
            if(GetSetting(QStringLiteral("[TimeSelection1]"), itemVal))
                TimeSelection[0] = (itemVal.toULong());

            itemVal = QString();
            if(GetSetting(QStringLiteral("[TimeSelection2]"), itemVal))
                TimeSelection[1] = (itemVal.toULong());

            itemVal = QString();
            if(GetSetting(QStringLiteral("[TimeSelection3]"), itemVal))
                TimeSelection[2] = (itemVal.toULong());

            itemVal = QString();
            if(GetSetting(QStringLiteral("[TimeSelection4]"), itemVal))
                TimeSelection[3] = (itemVal.toULong());

            itemVal = QString();
            if(GetSetting(QStringLiteral("[queueingMode]"), itemVal))
                queueingMode = (itemVal.toInt() == 1);
            else
                queueingMode = false; // Default

            itemVal = QString();
            if(GetSetting(QStringLiteral("[authStop]"), itemVal))
                authStop = (itemVal.toInt() == 1);
            else
                authStop = false; // Default

            itemVal = QString();
            if(GetSetting(QStringLiteral("[lowCurrentStop]"), itemVal))
                lowCurrentStop = (itemVal.toInt() == 1);
            else
                lowCurrentStop = false; // Default

            itemVal = QString();
            if(GetSetting(QStringLiteral("[noChargeToFull]"), itemVal))
                noChargeToFull = (itemVal.toInt() == 1);
            else
                noChargeToFull = false; // Default

            itemVal = QString();
            if(GetSetting(QStringLiteral("[unlockCarUnplug]"), itemVal))
                unlockCarUnplug = (itemVal.toInt() == 1);
            else
                unlockCarUnplug = false; // Default

            hardwareKeySupport = false;
            if (GetSetting(QStringLiteral("[KEYONLY]"), itemVal))
                hardwareKeySupport = (itemVal.compare("0") == 0)? false : true;
        }
public:
        void init()
        {
            // both ChargerConfig and QApplication are global variables.
            // Need explicit call to make sure the qApp is initialized
            qApp->installTranslator(&translator);
        }
    };

    void initChargerConfig()
    {
        ChargerConfiguration::instance().init();
    }

    bool ignoreThisVendorErrorCode(const std::string &errorcode)
    {
        bool ignore = false;

        (void)errorcode;
#if false
#warning "Overriding vendor error code"
        if (errorcode.compare("E-4083") == 0)
            ignore = true;
#endif
        return ignore;
    }

    bool overrideChargeTimeValue()
    {
#if false
#warning "Overriding ChargeTimeValue"
        return true;
#else
        return false;
#endif
    }
    unsigned long getOverrideChargeTimeValue()
    {
        return 60; // one-minute
    }
    bool enableFakeChargerLoad()
    {
        return false;
    }


    bool isStandaloneMode()
    {
        return ChargerConfiguration::instance().standaloneMode;
    }

    bool allowOfflineCharging()
    {
        return ChargerConfiguration::instance().offlineChargingSupported;
    }

    PayMethod getPayMethod()
    {
        return ChargerConfiguration::instance().pay;
    }

    ChargeMode getChargingMode()
    {
        return ChargerConfiguration::instance().chargeMode;
    }

    bool getWallSocketSupported()
    {
        return ChargerConfiguration::instance().cableConnType == CCharger::ConnectorType::WallSocket;
    }
    bool getAC63ANoLockSupported()
    {
        return ChargerConfiguration::instance().cableConnType == CCharger::ConnectorType::AC63A_NoLock;
    }

    bool isConnLockEnabled()
    {
        return ChargerConfiguration::instance().cableConnType != CCharger::ConnectorType::AC63A_NoLock;
    }

    bool getRfidSupported()
    {
        return ChargerConfiguration::instance().rfidSupported;
    }

    QString getOcppCsUrl()
    {
        return ChargerConfiguration::instance().ocppCsUrlStr;
    }

    std::string getChargePointSerialNum()
    {
        return ChargerConfiguration::instance().chargerSerNum;
    }

    std::string getChargerNum()
    {
        return ChargerConfiguration::instance().chargerNum;
    }

    std::string getChargerName()
    {
        return ChargerConfiguration::instance().chargerName;
    }

    bool forceOcppMeterValueKWh()
    {
        return ChargerConfiguration::instance().ocppMeterUnitKWh;
    }

    AcMeterPhase getMeterType()
    {
        return ChargerConfiguration::instance().meterPhase;
    }

    int getMeterPhase()
    {
        return (ChargerConfiguration::instance().meterPhase == AcMeterPhase::SPM93)? 3 : 1;
    }

    bool useSafeModeCurrentWhenOffline()
    {
        return ChargerConfiguration::instance().useSafeModeCurrent;
    }

    int getSafeModeCurrent()
    {
        return ChargerConfiguration::instance().safeModeCurrent;
    }

    int getMaxCutoffChargeCurrent()
    {
        return ChargerConfiguration::instance().maxChargeCurrent;
    }

    int getMinCutoffChargeCurrent()
    {
        return ChargerConfiguration::instance().minChargeCurrent;
    }

    int getCutoffDebouceTime()
    {
        return ChargerConfiguration::instance().minChargeDebouceTime;
    }

    bool reduceScreenTransitions()
    {
        return (!ChargerConfiguration::instance().hasDisplayUnit);
    }

    DisplayLang getCurrDisplayLang()
    {
        return ChargerConfiguration::instance().lang;
    }

    void setCurrDisplayLang(DisplayLang lang)
    {
        ChargerConfiguration::instance().lang = lang;
        switch (lang) {
        case DisplayLang::Chinese:
            if (!qApp->installTranslator(&ChargerConfiguration::instance().translator))
                qWarning() << "Install Translator failed!";
            break;
        case DisplayLang::English:
            if (!qApp->removeTranslator(&ChargerConfiguration::instance().translator))
                qWarning() << "Remove Translator failed!";
            break;
        default:
            break;
        }
    }

    DisplayLang toggleDisplayLang()
    {
        switch(ChargerConfiguration::instance().lang) {
        case charger::DisplayLang::English:
            setCurrDisplayLang(charger::DisplayLang::Chinese);
            break;
        default:
            setCurrDisplayLang(charger::DisplayLang::English);
            break;
        }
        return ChargerConfiguration::instance().lang;
    }

    int getAuthorizationCacheMaxSize()
    {
        return 1000;
    }

    QString getSoftwareVersion()
    {
        return VERSION;
    }

    bool passwordAuthEnabled()
    {
       return (charger::isStandaloneMode()) && (ChargerConfiguration::instance().rfidSupported == false)
               && (charger::getChargingMode() != charger::ChargeMode::NoAuthChargeToFull);
    }

    // user's choice at run-time
    charger::ChargeMode getCurrentChargingMode() {
        charger::ChargeMode chMode = charger::getChargingMode();
        if(m_ChargerA.IsChargingToFull())
            chMode = charger::ChargeMode::ChargeToFull;
        else
            chMode = charger::ChargeMode::UserSelect;

        return chMode;
    }

    bool isHardKeySupportRequired()
    {
        return ChargerConfiguration::instance().hardwareKeySupport;
    }

    //show load management msg?
    bool GetLoadManagementShow()
    {
        return ChargerConfiguration::instance().showloadmanagement;
    }

    int GetChargeTimeShow()
    {
        return ChargerConfiguration::instance().showChargingTime;
    }

    bool GetReaminTimeShow()
    {
        return ChargerConfiguration::instance().showRemainTime;
    }

    bool GetVoltageShow()
    {
        return ChargerConfiguration::instance().showVoltage;
    }

    bool GetCurrentShow()
    {
        return ChargerConfiguration::instance().showCurrent;
    }

    bool GetEnergyShow()
    {
        return ChargerConfiguration::instance().showEnergy;
    }

    bool GetSkipPayment()
    {
        return ChargerConfiguration::instance().skipPayment;
    }

    bool GetSinglePhaseDisplay()
    {
        return ChargerConfiguration::instance().singlePhaseDisplay;
    }

    int GetCTRatio()
    {
        return ChargerConfiguration::instance().CTRatio;
    }

    ulong * GetTimeSelection()
    {
        return ChargerConfiguration::instance().TimeSelection;
    }

    bool GetQueueingMode()
    {
        return ChargerConfiguration::instance().queueingMode;
    }

    bool GetAuthStop()
    {
        return ChargerConfiguration::instance().authStop;
    }

    bool GetLowCurrentStop()
    {
        return ChargerConfiguration::instance().lowCurrentStop;
    }

    bool GetNoChargeToFull()
    {
        return ChargerConfiguration::instance().noChargeToFull;
    }

    bool GetUnlockCarUnplug()
    {
        return ChargerConfiguration::instance().unlockCarUnplug;
    }

    int GetDistThreshold()
    {
        return ChargerConfiguration::instance().distThreshold;
    }

    int GetDistSenseTimes()
    {
        return ChargerConfiguration::instance().distSenseTimes;
    }

    bool GetKeepLockedWhenSuspended(){
        return ChargerConfiguration::instance().keepLockedWhenSuspended;
    }

    bool NeedRFIDCheckBeforeSuspend(){
        return ChargerConfiguration::instance().needRFIDCheckBeforeSuspend;
    }

    bool GetStopTransactionOnEVSideDisconnect(){
        return ChargerConfiguration::instance().stopTransactionOnEVSideDisconnect;
    }

    bool GetEnablePauseCharging(){
        return ChargerConfiguration::instance().enablePauseCharging;
    }
}
