#include "mainwindow.h"
#include "ui_mainwindow.h"
#ifdef WINDOWS_NATIVE_BUILD
#else
#include "wiringPi.h"
#endif

#include <qtimer.h>
#include "chargingwindow.h"
#include "messagewindow.h"
#include "waittingwindow.h"
#include "reservewindow.h"
#include "const.h"
#include <string>
#include <stdio.h>
#include <QTranslator>
#include <QComboBox>
#include <QDebug>
#include "cablewin.h"
#include "sockettype.h"
#include "./OCPP/ocppclient.h"
#include "ccharger.h"
#include "api_function.h"
#include "videoplay.h"
#include <QFile>
#include <QKeyEvent>
#include "hostplatform.h"
#include "firmwareupdatescheduler.h"
#include "logconfig.h"
#include "statusledctrl.h"
#include "popupmsgwin.h"
#include "distancesensorthread.h"
#include "gvar.h"

using namespace charger;

//=============================================================================

//全局变量:
QMainWindow *pCurWin    =NULL;  //记录当前打开的子窗口信息
QMainWindow *pMsgWin    =NULL;  //记录当前打开的信息窗口
QMainWindow *pVideoWin  =NULL;  //媒体播放窗口
QMainWindow *pPopupMsgWin  = NULL;
CCharger    &m_ChargerA = CCharger::instance();         //#1
OcppClient  &m_OcppClient = OcppClient::instance();

u16         m_IdleTime  =0;     //空闲无操作计时器, 控制弹出媒体播放
u16         m_ScreenSaveTime;   //屏保时间(弹出媒体播放) sec*2
u8          m_VideoNo   =0;     //0:屏保  1:教学视频
ulong       m_DelayTimer;       //延时充电计时器
uint        isSuspended  =0;    //非充电暂停状态
//=============================================================================

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    ocppConnRetryWaitInterval(15),
    ChargerThreadA(&stm32SerialPort),
    rebootPending(false),
    networkOfflineMsgWinTitle("P1.NET_OFF"),
    connUnavailMsgWinTitle("ConnectorUnavailable"),
    stationBusyMsgWinTitle("StationBusy"),
    popUpMsgWinTitle("Pop Up Msg"),
    connUnAvailPending(false),
    ocppTransRequester(EvChargingTransactionRegistry::instance(), m_OcppClient),
    pwrMeterRecSaveInterval(1), // every 15-minutes
    checkunlockTimeoutPeriod(std::chrono::seconds(6))
{
    qWarning() << "MAINWINDOW: init";
    ui->setupUi(this);

    ui->label_msg->setVisible(false);       // debug purpose
    ui->pushButton->setVisible(false);      //没用
    ui->pushButton_2->setVisible(false);    //没用

    updateStyleStrs(charger::getWallSocketSupported(), charger::getCurrDisplayLang());
    this->setStyleSheet(winByStyle);

    //{{TODO:
    //setStyleSheet("border-image: url(:/BK/ui_bk1.png);");
    //this->setStyleSheet ("venus--TitleBar {background-color: rgb(0,0,0);color: rgb(255,255,255);}");
    //ui->label->setAttribute(Qt::WA_TranslucentBackground, true);    //控件透明
    //ui->label_Txt->setAttribute(Qt::WA_TranslucentBackground, true);
    //ui->pushButton->setFlat(true);
    //ui->pushButton->setStyleSheet("border-image: url(:/BK/Desktop/flippybackground.png);");

    ui->label->setText(""); //No message
    QTimer *timer =new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(Blink()));
    connect(timer, SIGNAL(timeout()), this, SLOT(SearchCard()));
    connect(&ChargerThreadA, &CComPortThreadV2::ChargerModBus, &m_ChargerA, &CCharger::ChargerModBus);
    connect(&ChargerThreadA, &CComPortThreadV2::ChargerModBus2, &m_ChargerA, &CCharger::ChargerModBus2);
    timer->start(500);

    EvChargingTransactionRegistry &transactionRegistry = EvChargingTransactionRegistry::instance();
    transactionRegistry.setMeterSampleInterval(m_OcppClient.getMeterValueSampleInterval());

    connect(&ocppTransRequester, &OcppTransactionMessageRequester::transactionCompleted, this, &MainWindow::onTransactionCompleted);
    connect(&ocppTransRequester, &OcppTransactionMessageRequester::transactionError, this, &MainWindow::onTransactionError);
    connect(&m_OcppClient, &OcppClient::connected, this, &MainWindow::onOcppConnected);
    //connect(&m_OcppClient, &OcppClient::connectFailed, this, &MainWindow::onOcppConnectFailed);
    connect(&m_OcppClient, &OcppClient::callTimedout, this, &MainWindow::onOcppCallTimedout);
    //connect(&m_OcppClient, &OcppClient::disconnected, this, &MainWindow::onOcppDisconnected);
    connect(&m_OcppClient, &OcppClient::bootNotificationAccepted, this, &MainWindow::onOcppBootNotificationAccepted);
    connect(&m_OcppClient, &OcppClient::serverDateTimeReceived, this, &MainWindow::onServerDateTimeReceived);
    connect(&m_OcppClient, &OcppClient::meterSampleIntervalCfgChanged, this, &MainWindow::onMeterSampleIntervalCfgChanged);

    connect(&m_OcppClient, &OcppClient::hardreset, this, &MainWindow::hardreset);
    connect(&m_OcppClient, &OcppClient::popUpMsgReceived, this, &MainWindow::onPopUpMsgReceived);
    connect(&m_OcppClient, &OcppClient::cancelPopUpMsgReceived, this, &MainWindow::onCancelPopUpMsgReceived);
    connect(&closePopUpMsgTimer, &QTimer::timeout, this, &MainWindow::onCancelPopUpMsgReceived);
    connect(&m_OcppClient, &OcppClient::changeLedLight, this, &MainWindow::onChangeLedLight);

    FirmwareUpdateScheduler &fwupdater = FirmwareUpdateScheduler::getInstance();
    connect(&fwupdater, &FirmwareUpdateScheduler::downloadStarted, this, &MainWindow::onFirmwareDownloadStarted);
    connect(&fwupdater, &FirmwareUpdateScheduler::downloadReady, this, &MainWindow::onFirmwareDownloadCompleted);
    connect(&fwupdater, &FirmwareUpdateScheduler::downloadFailed, this, &MainWindow::onFirmwareDownloadFailed);
    connect(&fwupdater, &FirmwareUpdateScheduler::installReady, this, &MainWindow::onFirmwareInstallReady);
    connect(&fwupdater, &FirmwareUpdateScheduler::installFailed, this, &MainWindow::onFirmwareInstallFailed);
    connect(&fwupdater, &FirmwareUpdateScheduler::installCompleted, this, &MainWindow::onFirmwareInstallCompleted);
    ocppConnRetryTimer.setSingleShot(true);
    ocppConnRetryTimer.setInterval(std::chrono::duration_cast<std::chrono::milliseconds>(ocppConnRetryWaitInterval));
    connect(&ocppConnRetryTimer, &QTimer::timeout, this, &MainWindow::onOcppConnRetryWaited);
    connect(&m_ChargerA, &CCharger::connAvailabilityChanged, this, &MainWindow::onConnAvailabilityChanged);
    connect(&m_OcppClient, &OcppClient::scheduleConnUnavailChange, this, &MainWindow::onScheduleConnUnavailChange);
    connect(&m_OcppClient, &OcppClient::cancelScheduleConnUnavailChange, this, &MainWindow::onCancelScheduleConnUnavailChange);
    connect(&m_ChargerA, &CCharger::chargeCommandAccepted, this, &MainWindow::onChargeCommandAccepted);
    connect(&m_ChargerA, &CCharger::chargeCommandCanceled, this, &MainWindow::onChargeCommandCanceled);
    connect(&m_ChargerA, &CCharger::chargeFinished, this, &MainWindow::onChargeFinished);
    connect(&m_ChargerA, &CCharger::errorCodeChanged, this, &MainWindow::onChargerErrorCodeChanged);

    //stop resume charging transaction
    connect(&m_ChargerA, &CCharger::stopResumeChargeTransaction, this, &MainWindow::StopResumedChargingTransaction);

    //check ocpp unlock success or not
    checkunlockTimer.setSingleShot(true);
    checkunlockTimer.setInterval(checkunlockTimeoutPeriod);
    connect(&checkunlockTimer, &QTimer::timeout, this, &MainWindow::oncheckUnlockTimerEvent);
    connect(&m_OcppClient, &OcppClient::checkunlock, this, &MainWindow::checkunlock);

    pwrMeterRecSaveTimer.setInterval(pwrMeterRecSaveInterval);
    pwrMeterRecSaveTimer.setSingleShot(false); // periodic timer
    connect(&pwrMeterRecSaveTimer, &QTimer::timeout, this, &MainWindow::onPwrMeterRecSaveTimer);
    connect(&DistSensorThread, &DistanceSensorThread::parkingStateChanged,this,&MainWindow::onParkingStateChanged);


     offlinemessagelock = false;




#ifdef WINDOWS_NATIVE_BUILD
#else
    wiringPiSetupSys();     //用芯片脚标号
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
#endif
    DistSensorThread.StartSlave(200);  //"/dev/ttyUSB0"

    //no need to run extra thread anymore
    //ChargerThreadA.StartSlave(5000);

    QString itemvalue;
    if(!GetSetting(QStringLiteral("[SCREENSAVE]"), itemvalue)) itemvalue ="0";
    m_ScreenSaveTime =itemvalue.toInt() *2;
    GetSetting(QStringLiteral("[screensaver]"), itemvalue); //若无该项设置,则优先按[SCREENSAVE]设置
    if(itemvalue =="0") m_ScreenSaveTime =0;

    if (charger::isHardKeySupportRequired())
    {
        connect(&ChargerThreadA, SIGNAL(ExtKeyDown(char)), this, SLOT(ExtKeyDown(char)));
    }
}

MainWindow::~MainWindow()
{
    stm32SerialPort.close();
    delete ui;

    m_OcppClient.CloseWebSocket();
}

void MainWindow::updateStyleStrs(bool wallSocketSupported, charger::DisplayLang lang)
{
    using namespace charger;
    QString langInFn;
    switch (lang) {
    case DisplayLang::Chinese:
    case DisplayLang::SimplifiedChinese:
        langInFn = "ZH"; break;
    case DisplayLang::English: langInFn = "EN"; break;
    }

    QString fileBaseName;
    if (wallSocketSupported)
        fileBaseName = "socketTypeWin";
    else
        fileBaseName = "cableWin";

    winByStyle = QString("QWidget#centralWidget {background-image: url(%1%2.%3.jpg);}").arg(Platform::instance().guiImageFileBaseDir).arg(fileBaseName).arg(langInFn);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        updateStyleStrs(charger::getWallSocketSupported(), charger::getCurrDisplayLang());
        this->setStyleSheet(winByStyle);
    }
}

/*void MainWindow::mousePressEvent(QMouseEvent * event)
{
    //TODO: [先检查充电板可用]
    if(m_ChargerA.IsFaultState() ||m_ChargerA.IsBookingState())
    {
        //Nothing to do
    }
    else if(pCurWin ==NULL)
    {
        QString txt("");
        GetSetting(QStringLiteral("[SOCKET]"), txt);
        m_ChargerA.SocketType =0;
        if(txt =="1")
        {
            pCurWin =new SocketType(this);
        }
        else
        {
            pCurWin =new CableWin(this);
        }
        pCurWin->setWindowState(Qt::WindowFullScreen);
        pCurWin->show();
    }

    Q_UNUSED(event);
}*/


void MainWindow::displayOfflineMessage()
{
    if(offlinemessagelock != true)
    {
        qWarning() << "MAINWINDOW: Display offline message";
        pMsgWin = new MessageWindow(this);
        pMsgWin->setWindowState(charger::Platform::instance().defaultWinState);
        pMsgWin->setWindowTitle(networkOfflineMsgWinTitle);
        pMsgWin->show();
        offlinemessagelock = true;
    }
}

void MainWindow::closeOfflineMessage()
{
    if(pMsgWin && pMsgWin->windowTitle() == networkOfflineMsgWinTitle) {
        qWarning() << "MAINWINDOW: Close offline message";
        pMsgWin->close();
        pMsgWin = nullptr;
        offlinemessagelock = false;
    }
}


void MainWindow::displayConnUnavailMessage()
{
    qWarning() << "MAINWINDOW: Display connector unavailable message";
    // Display if not already - higher priority than other Msg window?
    if (pMsgWin != nullptr) {
        if (pMsgWin->windowTitle() == connUnavailMsgWinTitle)
            return; // Already in display
        else
            pMsgWin->close();
    }
    pMsgWin = new MessageWindow(this);
    pMsgWin->setWindowState(charger::Platform::instance().defaultWinState);
    pMsgWin->setWindowTitle(connUnavailMsgWinTitle);
    pMsgWin->show();
}

void MainWindow::closeConnUnavailMessage()
{
    if (pMsgWin != nullptr && pMsgWin->windowTitle() == connUnavailMsgWinTitle ) {
        qWarning() << "MAINWINDOW: Close connector unavailable message";
        pMsgWin->close();
        pMsgWin = nullptr;
    }
}

void MainWindow::displayStationBusyMessage()
{
    qWarning() << "MAINWINDOW: Display Station busy message";
    // Display if not already - higher priority than other Msg window?
    if (pMsgWin != nullptr) {
        if (pMsgWin->windowTitle() == stationBusyMsgWinTitle)
            return; // Already in display
        else
            pMsgWin->close();
    }
    pMsgWin = new MessageWindow(this);
    pMsgWin->setWindowState(charger::Platform::instance().defaultWinState);
    pMsgWin->setWindowTitle(stationBusyMsgWinTitle);
    pMsgWin->show();
}

void MainWindow::closeStationBusyMessage()
{
    if (pMsgWin != nullptr && pMsgWin->windowTitle() == stationBusyMsgWinTitle ) {
        qWarning() << "MAINWINDOW: Close station busy message";
        pMsgWin->close();
        pMsgWin = nullptr;
    }
}


void MainWindow::onOcppConnected()
{
    qWarning() << "MAINWINDOW: OCPP connected";
    // Stop connect-retry timer
    ocppConnRetryTimer.stop();

    closeOfflineMessage();
    //增加发一次状态:
    m_OcppClient.addStatusNotificationReq();
    //ocppTransRequester.nextTransaction();
    QTimer::singleShot(1000, this, &MainWindow::onCheckTransactionTimeout);
}

void MainWindow::onCheckTransactionTimeout()
{
    ocppTransRequester.nextTransaction();
}

void MainWindow::onOcppConnectFailed()
{
    qWarning() << "MAINWINDOW: OCPP connect fail";

    if((!charger::allowOfflineCharging()) && (pMsgWin ==NULL) && m_ChargerA.IsIdleState())
    {
        displayOfflineMessage();
    }
    // Set up connect-retry timer
    if (!ocppConnRetryTimer.isActive())
        ocppConnRetryTimer.start();
}

void MainWindow::onOcppDisconnected()
{
    qWarning() << "MAINWINDOW: OCPP disconnected";
    if((!charger::allowOfflineCharging()) && (pMsgWin ==NULL) && m_ChargerA.IsIdleState())
    {
        displayOfflineMessage();
    }
    // Set up connect-retry timer
    if (!ocppConnRetryTimer.isActive())
        ocppConnRetryTimer.start();
}

void MainWindow::onOcppCallTimedout()
{
    qWarning() << "MAINWINDOW: OCPP call timeout";
    // Disconnect and retry
    m_OcppClient.DisconnectServer();
    if (!ocppConnRetryTimer.isActive())
        ocppConnRetryTimer.start();
}

void MainWindow::onOcppConnRetryWaited()
{
    qWarning() << "MAINWINDOW: OCPP connect retry waited";
    EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
    if (reg.hasActiveTransaction() && !reg.activeTransactionHasTransId() && charger::getRfidSupported()) {
        qWarning(OcppComm) << "Charging started without Transaction-ID. Do not retry CS-connection until charging stops";
        ocppConnRetryTimer.start();
    }
    else {
        m_OcppClient.ConnectServer();
    }
}


void MainWindow::onMeterSampleIntervalCfgChanged()
{
    qWarning() << "MAINWINDOW: Meter sample interval config changed";
    EvChargingTransactionRegistry &transactionRegistry = EvChargingTransactionRegistry::instance();

    transactionRegistry.setMeterSampleInterval(m_OcppClient.getMeterValueSampleInterval());
}

void MainWindow::onOcppBootNotificationAccepted()
{
    qWarning() << "MAINWINDOW: OCPP boot noticfication accepted";
    FirmwareUpdateScheduler::getInstance().checkInstallStatus();
    m_OcppClient.addStatusNotificationReq();
}

void MainWindow::onServerDateTimeReceived(const QDateTime currTime)
{
    // update system time if it deviates more than 1 minute
    QDateTime utcNow = QDateTime::currentDateTimeUtc();
    if (qAbs(utcNow.secsTo(currTime)) >  Q_INT64_C(5)) {
        qWarning() << "MAINWINDOW: Update datetime from server";
        charger::Platform::instance().updateSystemTime(currTime);
    }
}

void MainWindow::onFirmwareDownloadStarted()
{
    qWarning() << "MAINWINDOW: Firmware download started";
    qCDebug(FirmwareUpdate) << "FirmwareStatus = Downloading";
    m_OcppClient.addFirmwareStatusNotificationReq(ocpp::FirmwareStatus::Downloading);
}

void MainWindow::onFirmwareDownloadCompleted(const QString &localPath)
{
    qWarning() << "MAINWINDOW: Firmware download completed";
     qCDebug(FirmwareUpdate) << "FirmwareStatus = Downloaded to " << localPath;
     m_OcppClient.addFirmwareStatusNotificationReq(ocpp::FirmwareStatus::Downloaded);
}

void MainWindow::onFirmwareDownloadFailed(const QString &errStr)
{
    qWarning() << "MAINWINDOW: Firmware download failed";
    qCCritical(FirmwareUpdate) << "FirmwareStatus = DownloadFailed - " << errStr;
    m_OcppClient.addFirmwareStatusNotificationReq(ocpp::FirmwareStatus::DownloadFailed);
}

void MainWindow::onFirmwareInstallReady(const QString &localPath)
{
    qWarning() << "MAINWINDOW: Firmware install ready";
    qCDebug(FirmwareUpdate) << "FirmwareStatus = Installing from " << localPath;
    m_OcppClient.addFirmwareStatusNotificationReq(ocpp::FirmwareStatus::Installing);
    rebootPending = true;
}

void MainWindow::onFirmwareInstallFailed(const QString &errStr)
{
    qWarning() << "MAINWINDOW: Firmware install Failed";
    qCCritical(FirmwareUpdate) << "FirmwareStatus = InstallFailed - " << errStr;
    m_OcppClient.addFirmwareStatusNotificationReq(ocpp::FirmwareStatus::InstallationFailed);
}

void MainWindow::onFirmwareInstallCompleted()
{
    qWarning() << "MAINWINDOW: Firmware install completed";
    qCCritical(FirmwareUpdate) << "FirmwareStatus = InstallCompleted";
    m_OcppClient.addFirmwareStatusNotificationReq(ocpp::FirmwareStatus::Installed);
}

void MainWindow::onConnAvailabilityChanged(bool newVal)
{
    qWarning() << "MAINWINDOW: Connector availability changed. Val:" << newVal;
    if (newVal) {
        closeConnUnavailMessage();
    }
    else {
        displayConnUnavailMessage();
    }
}


void MainWindow::onChargeCommandAccepted(QDateTime startDt, unsigned long startMeterVal, bool isResumedCharging, int resumedTransId)
{
    if(resumedTransId < 0 && isResumedCharging)return;
    qWarning() << "MAINWINDOW: Charge command accepted. Start meter value: " << startMeterVal << " Resume charge: " << isResumedCharging << " Resume transaction id: " << resumedTransId;
    EvChargingTransactionRegistry &transactionRegistry = EvChargingTransactionRegistry::instance();
   qWarning("onChargeCommandAccepted");
   qWarning()<<isResumedCharging;
   qWarning()<<resumedTransId;
    if (isResumedCharging) {
        // TBD: past metervalues are lost after resume-charging
        transactionRegistry.addActive(startDt, startMeterVal, m_ChargerA.IdTag,
        {}, resumedTransId >= 0? QString::number(resumedTransId) : QString());
        qWarning("yes");
    }
    else {
        transactionRegistry.startNew(startDt, startMeterVal, m_ChargerA.IdTag);
        qWarning("no");
    }
    ocppTransRequester.nextTransaction();
}

void MainWindow::onChargeCommandCanceled(QDateTime canceledDt, unsigned long cancelMeterVal, ocpp::Reason cancelReason)
{
    qDebug() << "MAINWINDOW: Charge command canceled";
    EvChargingTransactionRegistry &transactionRegistry = EvChargingTransactionRegistry::instance();

    transactionRegistry.cancel(canceledDt, cancelMeterVal, cancelReason);
}

void MainWindow::onChargeFinished(QDateTime stopDt, unsigned long stopMeterVal, ocpp::Reason stopReason)
{
    qWarning() << "MAINWINDOW: Charge finished";
    EvChargingTransactionRegistry &transactionRegistry = EvChargingTransactionRegistry::instance();

    transactionRegistry.finish(stopDt, stopMeterVal, stopReason);
    isSuspended = 0;
}

void MainWindow::onChargerErrorCodeChanged()
{
    qWarning() << "MAINWINDOW: Charger error code changed";
    m_OcppClient.addStatusNotificationReq();
}

void MainWindow::onTransactionCompleted(const QString transactionId)// Start, MeterValue(s), Stop
{
    qWarning() << "MAINWINDOW: Transaction completed. ID: " << transactionId;
    bool removed = false;
    ocppTransRequester.reset();
    EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
    EvChargingTransaction::currentTransc = nullptr;
    if (!(transactionId.isNull() || transactionId.isEmpty())) {
        TransactionRecordFile recFile{charger::getChargerNum().c_str(), charger::getMeterType()};
        reg.saveTransactionToRecord(transactionId, recFile);
        removed = reg.removeByTransactionId(transactionId);
        if (!removed) {
            qCWarning(Transactions) << "Unable to remove the completed transaction (id=" << transactionId << ") from registry.";
        }
    }
    else {
        qCWarning(Transactions) << "MainWindow::onTransactionCompleted(): transaction-Id is null-value.";
    }

    ocppTransRequester.nextTransaction();
}

void MainWindow::onTransactionError(const QString idTag, const QString transactionId)
{
    qWarning() << "MAINWINDOW: Transaction error. Id tag: " << idTag << " Transaction id: " << transactionId;
    bool removed = false;
    ocppTransRequester.reset();
    EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
    if (!(idTag.isNull() || idTag.isEmpty())) {
        removed = reg.removeByAuthId(idTag);
    }
    if (!removed && !(transactionId.isNull() || transactionId.isEmpty())) {
        removed = reg.removeByTransactionId(transactionId);
    }
    if (!removed) {
        qCWarning(Transactions) << "Unable to remove the failed transaction (id=" << transactionId << ") from registry.";
    }
    ocppTransRequester.nextTransaction();
}

void MainWindow::onPwrMeterRecSaveTimer()
{
    qWarning() << "MAINWINDOW: Power meter record save timer";
    PowerMeterRecFile recFile{charger::getChargerNum().c_str(), charger::getMeterPhase()};
    m_ChargerA.SavePowerMeterRecord(recFile);
}

void MainWindow::Blink(void)
{
    // Can we reboot now?
    if (rebootPending) {
        qWarning() << "MainWindow(Blink): reboot pending";
        if (!m_ChargerA.IsPreparingState() && !m_ChargerA.IsBookingState() && m_ChargerA.IsIdleState()) {
            EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
            if (!reg.hasActiveTransaction()) {
                qCDebug(FirmwareUpdate) << "Scheduling Reboot...";
                qWarning() << "MAINWINDOW(Blink): Scheduling reboot";
                charger::Platform::instance().scheduleReboot();
            }
        }
    }

    // Can we make Connector Unavailable now?
    if (connUnAvailPending) {
        switch (m_ChargerA.ChangeConnAvail(false)) {
        case CCharger::Response::OK:
            connUnAvailPending = false; // applied
            break;
        case CCharger::Response::Denied:
            connUnAvailPending = false; // cannot apply
            break;
        case CCharger::Response::Busy:
            // try again later
            break;
        }
    }

    if(ui->label->isHidden())
    {
        ui->label->show();
    }
    else
    {
        ui->label->hide();
    }

    // Only open CableWin and SocketWin if Connector is Available
    /*
    if (!m_ChargerA.IsConnectorAvailable()) {
        displayConnUnavailMessage();
    }
    else if(!charger::isStandaloneMode()) {
        if (m_OcppClient.isOffline()) {
            closeStationBusyMessage();
        }
        else if (ocppTransRequester.isProcessingInactiveTrans()) {
            // online and processing previous transactions
            displayStationBusyMessage();
        }
        else {
            // online and not processing previous transactions
            closeStationBusyMessage();
        }
    }*/
    if(charger::isStandaloneMode()){
        closeStationBusyMessage();
    }

    if(m_ChargerA.IsBookingState() && m_ChargerA.VendorErrorCode == "")
    {
        if((!m_ChargerA.IsCablePlugIn()) && ((pCurWin ==NULL) || (pCurWin->windowTitle() =="SocketTypeW") || (pCurWin->windowTitle() =="CableW")))
        { //待机且没插枪时

            qWarning() << "MainWindow: Set booking window";
            QMainWindow *pWin =new reservewindow(this);
            pWin->setWindowState(Platform::instance().defaultWinState);
            pWin->show();

            if(pCurWin)
            {
                pCurWin->close();
                pCurWin =NULL;
            }
            pCurWin = pWin;
        }

        if(m_ChargerA.ExpiryDate <QDateTime::currentDateTime())
        {
            m_ChargerA.ChargerIdle();
        }
    }
    else
    {
        // qWarning() << "MainWindow: Cancel booking window";
        // pCurWin->close();
        // pCurWin = NULL;
    }

    // in case the above display pMsgWin and should fail the following condition
    if((pCurWin ==NULL) && (pMsgWin ==NULL) && m_ChargerA.Initialized)
    {
        if (m_ChargerA.IsResumedCharging()) {
            // Show waiting screen
            pCurWin = new WaittingWindow(this->parentWidget());
        }
        else if(charger::getWallSocketSupported())
        {
            SocketType *socketWin =new SocketType(this);
            pCurWin = socketWin;
        }
        else
        {
            CableWin *cableWin =new CableWin(this);
            pCurWin = cableWin;
        }
        pCurWin->setWindowState(Platform::instance().defaultWinState);
        pCurWin->show();

//        if(charger::GetQueueingMode())
//        {
//            if(m_ChargerA.VendorErrorCode == "")
//            {
//                qWarning() << "queueing mode, change LED light to blue";
//                StatusLedCtrl::instance().setState(StatusLedCtrl::State::blue);
//            }
//            else
//            {
//                qWarning() << "queueing mode, error, change LED light";
//                StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashingred);
//            }
//        }
//        else if(m_ChargerA.VendorErrorCode == "")
//        {
//            qWarning() << "not queueing mode, change LED light to green";
//            StatusLedCtrl::instance().setState(StatusLedCtrl::State::green);
//        }
    }


//------------------------------------------------
    m_ChargerA.ConnectTimeout++;
    m_ChargerA.LockTimeout++;
    if(m_ChargerA.CommandTimeout <600) m_ChargerA.CommandTimeout++;
    if(m_ChargerA.ConnectTimeout >16)
    {
        if(!m_ChargerA.Initialized) m_ChargerA.Initialized =true;
    }

#if(0)  //不用这里,	在Refresh已有
    m_ChargerA.SoftPCB();
#endif
//qDebug() <<"main timer command:"<<QString("%1").arg((int)m_ChargerA.Command, 0, 16).toUpper()<<"========" <<pCurWin;
    if(m_ChargerA.Initialized)
    {
        ChargerControl();

        //
        // Start saving power-meter data to file
        //
        if (!pwrMeterRecSaveTimer.isActive())
            pwrMeterRecSaveTimer.start();

        //
        // Start Connecting to CS if this is an OCPP-installation
        //
        if(!charger::isStandaloneMode() && !m_OcppClient.isConnectedOrConnecting()) {
            if (!ocppConnRetryTimer.isActive()) // do not interfere with ocppConnRetryTimer
                m_OcppClient.ConnectServer();
        }

        if(!charger::isStandaloneMode())
            m_OcppClient.OcppManage();

        if(m_ChargerA.VendorErrorCode !="")
        {
            /* PC-Development only: Filter E-4083 for now */
            if(pMsgWin == NULL)
            {
                if (charger::ignoreThisVendorErrorCode(m_ChargerA.VendorErrorCode.toStdString())) {
                    qWarning() << "Skipping display of VendorErrorCode: " << m_ChargerA.VendorErrorCode << endl;
                }
                else {
                    pMsgWin =new MessageWindow(this);
                    pMsgWin->setWindowState(Platform::instance().defaultWinState);
                    pMsgWin->show();
                }
            }
            StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashingred);
            //qWarning() << "Flashingred Show VendorErrorCode: " << m_ChargerA.VendorErrorCode << endl;
        }
        if(pMsgWin) ((MessageWindow *)pMsgWin)->UpdateMessage();

        //test:
        if(m_OcppClient.isChargerAccepted())
            ui->label_msg->setText("Connecting..."); //DEL
        else
            ui->label_msg->setText(QString("%1-%2").arg(m_ChargerA.State, 8, 16, QLatin1Char('0')).arg((int)m_ChargerA.Command, 0, 16).toUpper());
        //test show:
        //qDebug() <<ui->label_msg->text();
    }
//------------------------------------------------

    if(pMsgWin ||m_ChargerA.IsBookingState() ||m_ChargerA.IsCablePlugIn() ||((m_ChargerA.ConnType() == CCharger::ConnectorType::WallSocket)&&!m_ChargerA.IsIdleState())) m_IdleTime =0;  //有其它窗口或有操作时重计时
    if((++m_IdleTime >m_ScreenSaveTime) &&(m_ScreenSaveTime))
    {
        m_IdleTime  =m_ScreenSaveTime;
        if(!pVideoWin)
        {
            if(pCurWin)
                pVideoWin =new VideoPlay(pCurWin);
            else
                pVideoWin =new VideoPlay(this);
            pVideoWin->setWindowState(Qt::WindowFullScreen);
            pVideoWin->show();
        }
    }
    else
    {
        if(pVideoWin && (m_IdleTime ==1))
        {
            pVideoWin->close();
            pVideoWin =NULL;
        }
    }
    //qDebug() <<m_IdleTime <<"==" <<pVideoWin; //***test

    //POP Setup:
    if(ui->pushButton_2->isDown())
    {
        //ui->label->setText("down");
    }
    else
    {
        //ui->label->setText("up");
    }

}

void MainWindow::on_FoundCard(void)
{ //读到卡
    qWarning() << "MAINWINDOW: Found card";
    ChargingWindow  *newWindow;

    newWindow =new ChargingWindow();   //ChargingWindow(this)
    //newWindow->setWindowState(Qt::WindowFullScreen);
    //newWindow->showFullScreen();
    newWindow->show();
}

void MainWindow::SearchCard(void)
{ //读卡
//    int num;
//    char msg[20];
//    QString txt;

//    num =(ui->label_2->text()).toInt();
//    num++;
//    txt.setNum(num);
//    sprintf(msg, "%d", num);
//    ui->label_2->setText(txt);
//    //ui->label->setText(msg);
}

void MainWindow::on_pushButton_clicked()
{
  //  this->close();
}

void MainWindow::ChargerControl()
{
    ocpp::ChargerStatus newstatus;

    newstatus =m_ChargerA.StatusManage();
    if(newstatus !=m_ChargerA.Status)
    {
        if(newstatus==ocpp::ChargerStatus::Available && m_ChargerA.Status==ocpp::ChargerStatus::Finishing ){
            emit m_ChargerA.chargeFinished(m_ChargerA.StopDt, m_ChargerA.MeterStop, m_ChargerA.StopReason);
        }
        qDebug() << QString("ChargerStatusChanged: %1 -> %2")
                    .arg(ocpp::ChargerStatusStrs.at(m_ChargerA.Status).data())
                    .arg(ocpp::ChargerStatusStrs.at(newstatus).data());
        m_ChargerA.OldStatus = m_ChargerA.Status;
        m_ChargerA.Status = newstatus;
        m_OcppClient.addStatusNotificationReq();
        if((m_ChargerA.Status ==ocpp::ChargerStatus::Charging || m_ChargerA.Status == ocpp::ChargerStatus::SuspendedEVSE)
                && isSuspended ==0)
        {
            //if(m_ChargerA.TransactionId ==-1) m_ChargerA.StatusChanged =false;    //后发CHARGING状态

            QMainWindow * pWin = new ChargingWindow(this);
            pWin->setWindowState(charger::Platform::instance().defaultWinState);
            pWin->show();

            if(pCurWin)
            {
                pCurWin->close();
                pCurWin =NULL;
            }

            if(pMsgWin)
            {
                pMsgWin->close();
                pMsgWin =NULL;
            }

            pCurWin =pWin;
            qWarning() << "charging, change LED light to flashing green";
            StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashinggreen);
        }
    }

    // The following block catches where the charging is unable to start because the charging
    // cable cannot be locked.
    if(((m_ChargerA.Command &_C_CHARGEMASK) ==_C_CHARGECOMMAND) && (m_ChargerA.CommandTimeout > 6)
            && ((m_ChargerA.Status ==ocpp::ChargerStatus::Preparing) || m_ChargerA.IsSuspendedEVorEVSE())) {
        qDebug() << "Charging-Command Sent - but still not in Charging-State.";
        if (charger::isConnLockEnabled() && m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected() && !m_ChargerA.IsFaultState()) {
            if (m_ChargerA.IsUnLock()) {
                qWarning() << "Charger is still unlock? " << (m_ChargerA.IsUnLock()? "Y" : "N");
                if (pMsgWin == nullptr) {
                    pMsgWin =new MessageWindow(this);
                    pMsgWin->setWindowState(charger::Platform::instance().defaultWinState);
                    pMsgWin->setWindowTitle("P7.LOCK");
                    pMsgWin->show();
                }
            }
            else {
                if (pMsgWin && (pMsgWin->windowTitle() == "P7.LOCK"))
                    pMsgWin->close();
            }
        }
    }
    else
    {
        if (pMsgWin && (pMsgWin->windowTitle() == "P7.LOCK"))
            pMsgWin->close();
    }

    //{{发送新命令:
    QByteArray command;     //Command-u8 u8 u32 u32 u32 u32 u8 u8 长度为9word
    /*
    command.append(m_ChargerA.Command |(static_cast<unsigned char>(m_ChargerA.ConnType()) <<5));
    int current = m_ChargerA.GetCurrentLimit();

    // Overridden with SafeCurrent if applicable
    if(!charger::isStandaloneMode() && useSafeModeCurrentWhenOffline() && m_OcppClient.isOffline())
        current = charger::getSafeModeCurrent(); // Override it no-matter what. (Trust the Safe-Mode-Current value)
    qCDebug(ChargerMessages) << "Command: MaxCurrent Set to " << current;
    command.append(char(current));          //传送限定电流值

    u32 regtmp32;
    if((m_ChargerA.Command &_C_CHARGEDT) !=_C_CHARGEDT)
    {
        regtmp32 =m_ChargerA.RemainTime;
        command.append((char*)&regtmp32, 4);
        regtmp32 =m_ChargerA.ChargeQValue;
        command.append((char*)&regtmp32, 4);
    }
    else
    {
        // Convert to Local Time first?
        QDate localStartDate = m_ChargerA.StartDt.toLocalTime().date();
        // Append "YYYYMMDD" to command
        QString dateStr = localStartDate.toString("yyyyMMdd");
        command.append(dateStr.toLocal8Bit());
    }

    regtmp32 =0;        //已充值
    command.append((char*)&regtmp32, 4);
    command.append((char*)&regtmp32, 4);
    command.append(static_cast<int>(charger::getMeterType()));
    command.append(static_cast<int>(StatusLedCtrl::instance().getState()));
    */

    int current = m_ChargerA.GetCurrentLimit();
    // Overridden with SafeCurrent if applicable
    if(!charger::isStandaloneMode() && useSafeModeCurrentWhenOffline() && m_OcppClient.isOffline())
        current = charger::getSafeModeCurrent(); // Override it no-matter what. (Trust the Safe-Mode-Current value)

    CHARGE_COMMAND ccmd = {
        m_ChargerA.Command,
        static_cast<unsigned char>(m_ChargerA.ConnType()),
        u8(current),
        m_ChargerA.RemainTime,
        m_ChargerA.ChargeQValue,
        "",
        static_cast<u16>(charger::getMeterType()),
        static_cast<u16>(StatusLedCtrl::instance().getState())
    };
    command.append(reinterpret_cast<char *>(&(ccmd.commandBody)),sizeof(u8));
    command.append(reinterpret_cast<char *>(&(ccmd.connectorType)),sizeof(u8));
    command.append(reinterpret_cast<char *>(&(ccmd.limitedCurrent)),sizeof(u8));
    command.append(reinterpret_cast<char *>(&(ccmd.remainTime)),sizeof(u32));
    command.append(reinterpret_cast<char *>(&(ccmd.chargeQValue)),sizeof(u32));
    command.append(reinterpret_cast<char *>(&(ccmd.dateStr)),8);
    command.append(reinterpret_cast<char *>(&(ccmd.meterType)),sizeof(u16));
    command.append(reinterpret_cast<char *>(&(ccmd.ledState)),sizeof(u16));

    //memcpy(ccmd.dateStr, m_ChargerA.StartDt.toLocalTime().date().toString("yyyyMMdd").toLocal8Bit().data(), 8);
    //tlv::TlvBox t;
    //t.PutBytesValue(0x0301,reinterpret_cast<uint8_t *>(&ccmd),sizeof(CHARGE_COMMAND));
    //ChargerThreadA.SetNewCommand(t.convertToByteArray());      //长度自动计算, 不用设置
    ChargerThreadA.SetNewCommand(command);      //长度自动计算, 不用设置

    //qDebug()<<"Pi command:"<<QString("%1").arg((int)m_ChargerA.Command, 0, 16).toUpper();
    //qDebug()<<"Pi command(raw):"<<command.toHex();
}

void MainWindow::ExtKeyDown(const char keycode)
{
    QWidget *w =QApplication::activeWindow();
    if(w)
    {
        //qDebug()<<w->objectName();
        if(keycode =='M')
            QCoreApplication::postEvent(w, new QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier));
        else if (keycode =='\n')
            QCoreApplication::postEvent(w, new QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier));
    }
    m_IdleTime =1;      //can not 0
}

void MainWindow::StopResumedChargingTransaction(const QString transId, const QDateTime stopDt, const unsigned long stopMeterVal, const ocpp::Reason reason, const QString tagId)
{
    qWarning() << "MAINWINDOW: Stop resumed charging transaction. Transaction id: " << transId << " Reason: " << ocpp::reasonStrs.at(reason).data() << " tagId: " << tagId;
    m_OcppClient.addStopTransactionReq(transId, stopDt, stopMeterVal, reason, tagId);
}

void MainWindow::checkunlock()
{
    if (!checkunlockTimer.isActive()) {
        checkunlockTimer.start();
    }
}

void MainWindow::oncheckUnlockTimerEvent()
{
    if (!m_ChargerA.IsUnLock()) {
        //弹出窗口方式提示卡锁:
        if(!pMsgWin)
        {
            m_ChargerA.VendorErrorCode ="";
            pMsgWin =new MessageWindow(this);
            pMsgWin->setWindowState(charger::Platform::instance().defaultWinState);
            pMsgWin->setWindowTitle("P7.UNLOCK");
            pMsgWin->show();
        }
    }
}

void MainWindow::hardreset()
{
    qWarning() << "MAINWINDOW: hard reset";
    m_ChargerA.StopCharge();
    charger::Platform::instance().hardreset();
}

void MainWindow::onParkingStateChanged(QString pState,float dist)
{
    //qWarning()<<"parking state change!";
    m_OcppClient.addParkingDataTransferReq(pState, dist);
}

void MainWindow::onPopUpMsgReceived()
{
    qWarning() << "MAINWINDOW: open pop up msg window";

    if (pPopupMsgWin)
    {
//        pPopupMsgWin->close();
//        pPopupMsgWin = nullptr;
        ((popupMsgWin *)pPopupMsgWin)->updateContent();
        pPopupMsgWin->show();
        pCurWin->hide();
    }
    else
    {
        pPopupMsgWin = new popupMsgWin(this);
        pPopupMsgWin->setWindowState(charger::Platform::instance().defaultWinState);
        pPopupMsgWin->setWindowTitle(popUpMsgWinTitle);
        pPopupMsgWin->show();
        pCurWin->hide();
    }


    if (closePopUpMsgTimer.isActive())
                closePopUpMsgTimer.stop();

    closePopUpMsgTimer.setSingleShot(true);
    closePopUpMsgTimer.setInterval(m_ChargerA.popupMsgDuration * 1000);

    if (!closePopUpMsgTimer.isActive())
                closePopUpMsgTimer.start();

}

void MainWindow::onCancelPopUpMsgReceived()
{
    if (pPopupMsgWin)
    {
        qWarning() << "MAINWINDOW: Close pop up msg window";
        pPopupMsgWin->close();
        pPopupMsgWin = nullptr;
        pCurWin->show();
    }
    if (closePopUpMsgTimer.isActive())
    {
        closePopUpMsgTimer.stop();
    }
}

void MainWindow::onChangeLedLight(int color)
{
    StatusLedCtrl::instance().setState(static_cast<StatusLedCtrl::State>(color));
}
