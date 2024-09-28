#include "chargingwindow.h"
#include "ui_chargingwindow.h"
#include "chargefinishwindow.h"
#include "messagewindow.h"
#include <QtGui>
//#include <QUrl>
#include <QWidget>
#include <QMessageBox>
#include "ccharger.h"
#include "const.h"
#include "rfidwindow.h"
#include "chargerequestpassword.h"
#include "chargerrequestrfid.h"
#include "gvar.h"
#include "api_function.h"
#include "hostplatform.h"
#include "evchargingtransactionregistry.h"

#define _C_SECOND_OFFSET    59

static bool forceSinglePhaseDisplay = false;

ChargingWindow::ChargingWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChargingWindow),
    m_SaveTime(16),
    stopChargingRequested(false),
    waitRfidReleaseConn(false),
    minCurrentDebounceCnt(0),
    unlockTimeoutPeriod(6),
    connIcon(m_OcppClient)
{
    qWarning() << "CHARGING WINDOW: init";
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);
    // disable and hide unlock-button
    ui->unlockButton->setEnabled(false);
    ui->unlockButton->setVisible(false);

    connect(&chargeMonitorTimer, &QTimer::timeout, this, &ChargingWindow::onChargeMonitorTimerEvent);
    chargeMonitorTimer.start(2000);

    unlockTimer.setSingleShot(true);
    unlockTimer.setInterval(unlockTimeoutPeriod);
    connect(&unlockTimer, &QTimer::timeout, this, &ChargingWindow::onUnlockTimerEvent);

    charger::ChargeMode chMode = charger::getCurrentChargingMode();

    forceSinglePhaseDisplay = charger::GetSinglePhaseDisplay();

    buildDetailsGridUI(chMode, forceSinglePhaseDisplay? 1 : charger::getMeterPhase());

    updateStyleStrings(charger::getCurrDisplayLang(), chMode);
    // Window Background
    this->setStyleSheet(winBgStyle);
    ui->statusLabel->setText(tr("Charging..."));

    // Safe-Mode-Charging label
    ui->label_SafeMode->setText(tr("Safe-Mode"));
    ui->label_SafeMode->setStyleSheet(safeModeLabelStyle);

    switch (chMode) {
    case charger::ChargeMode::ChargeToFull:
    case charger::ChargeMode::UserSelect:
        if (charger::isStandaloneMode()) {
            // standalone-mode
            prepareStandaloneModeAuthWin();
        }
        else {
            charger::PayMethod pay = charger::getPayMethod();
            switch (pay) {
            case charger::PayMethod::RFID:
                //Note: the WaitingWindow::rfidAuthorizer could be holding the
                // RFID-Card-Reader Serial-Port. But since that thread will
                // keep retrying on open-failures, this rfidAuthorizer will
                // eventually be able to open the RFID-reader once WaitingWindow
                // closed.
                rfidAuthorizer = std::make_unique<ChargerRequestRfid>(parent);
                connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestAccepted, this, &ChargingWindow::onRfidAuthAccepted);
                connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestCanceled, this, &ChargingWindow::onRfidAuthCanceled);
                rfidAuthorizer->setStartTxRfid(m_ChargerA.IdTag);
                rfidAuthorizer->start(ChargerRequestRfid::Purpose::EndCharge, false); // listen to RFID card in the background
                break;
            default:
                break;
            }
        }
        break;
    default:
        // Do nothing
        break;
    }

    if(charger::GetChargeTimeShow() == 1)
    {
        updateTimeLabel(chargeHourValLabel, chargeMinuteValLabel, m_ChargerA.ChargeTimeValue);
    }
    if(charger::GetChargeTimeShow() == 2)
    {
        updateTimeLabel(chargeHourValLabel, chargeMinuteValLabel, 0);
    }

    if(charger::GetReaminTimeShow() == true)
    {
        updateTimeLabel(remainHourValLabel, remainMinuteValLabel, m_ChargerA.ChargeTimeValue);
    }

    m_ChargerA.NewRecord();

    updateButtonStates();

    ui->label_SafeMode->setVisible(false);  //指示是否在执行安全电流充电

    connect(&m_ChargerA, &CCharger::stopChargeRequested, this, &ChargingWindow::onChargerStopChargeRequested);

    ++instanceCount;
    objectNum = instanceCount;
    objectPool.append(this);
}

void ChargingWindow::updateTimeLabel(QLabel &hhLabel, QLabel &mmLabel, unsigned long timeVal)
{
    int hh = (timeVal + _C_SECOND_OFFSET) / 3600;
    int mm = (timeVal + _C_SECOND_OFFSET - hh*3600) / 60;
    QString hhStr = QString("%1").arg(hh, 2, 10, QLatin1Char('0'));
    QString mmStr = QString("%1").arg(mm, 2, 10, QLatin1Char('0'));
    hhLabel.setText(hhStr);
    mmLabel.setText(mmStr);
}

ChargingWindow::~ChargingWindow()
{
    delete ui;
    if (chargeMonitorTimer.isActive())
        chargeMonitorTimer.stop();

    if(pMsgWin) pMsgWin->close();
//    if(charger::getRfidSupported())
//        if(pPwdWin) pPwdWin->close();
    --instanceCount;
    objectPool.removeOne(this);
}

void ChargingWindow::updateStyleStrings(enum charger::DisplayLang lang, enum charger::ChargeMode chargeMode)
{
    using namespace charger;

    QString langInFn;
    switch (lang)
    {
    case DisplayLang::English: langInFn = "EN"; break;
    case DisplayLang::Chinese:
    case DisplayLang::SimplifiedChinese:
        langInFn = "ZH"; break;
    }

    QString chModeInFn;
    switch (chargeMode)
    {
    case ChargeMode::ChargeToFull: chModeInFn = "Auto"; break;
    case ChargeMode::UserSelect: chModeInFn = "Timer"; break;
    default:
        throw "Unexpected ChargeMode value here\n";
    }

    winBgStyle = QString("QWidget#centralwidget {background-image: url(%1chargingWin.jpg);}").arg(Platform::instance().guiImageFileBaseDir);
    exitLabelStyle = QString("background-image: url(%1PNG/stopCharging.%2.png);").arg(Platform::instance().guiImageFileBaseDir).arg(langInFn);
    exitButtonStyle = QString("QPushButton{background:transparent; image:none;} QPushButton:checked{border-image: url(%1PNG/Rect2.png);}").arg(Platform::instance().guiImageFileBaseDir);
    safeModeLabelStyle = QString("color:red; font-family: Piboto, Calibri, sans-serif; font-size: 22px");
    loadmanageStyle = QString("qproperty-alignment: AlignCenter; color:white; font-family: Piboto, Calibri, sans-serif; font-size: 22px");
    pauseLabelStyle = QString("background-image: url(%1PNG/pauseCharging.%2.png);").arg(Platform::instance().guiImageFileBaseDir).arg(langInFn);
    pauseButtonStyle = QString("QPushButton{background:transparent; image:none;} QPushButton:checked{border-image: url(%1PNG/Rect2.png);}").arg(Platform::instance().guiImageFileBaseDir);
}

void ChargingWindow::setGridLabel(QLabel &label, QString text, Qt::Alignment align)
{
    label.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    label.setText(text);
#if false // show frame to debug layout issues.
    label.setFrameStyle(QFrame::Box|QFrame::Plain);
    label.setLineWidth(1);
#endif
    label.setAlignment(align);
}

// assuming phaseNum is zero-based.
QString ChargingWindow::makeDisplayTextOfPhaseValues(const double val, const int phaseNum, const int decPlaces, const int numPhases)
{
    QString phasePrefix;
    if (numPhases > 1) {
        phasePrefix = QString("L%1-").arg(phaseNum + 1); // starts with 1 instead of 0.
    }
    QString dispVal = QString("%1%2").arg(phasePrefix).arg(val, 0, 'f', decPlaces);
    if ((numPhases > 0) && phaseNum < (numPhases-1)) {
        // see what's like without the slash
        //dispVal += " /";
    }
    return dispVal;
}

void ChargingWindow::buildDetailsGridUI(enum charger::ChargeMode chargeMode, int phases)
{
    QString gridStyle = QString("background:transparent; image:none; color:white; font-family: Piboto, Calibri, sans-serif; font-size: 22px");
    ui->chargeDetailsWidget->setStyleSheet(gridStyle);
    ui->chargeDetailsWidget->setLayout(&chargeDetailsGrid);

    // apply the right-alignment of row-heading when QGridLayout::AddWidget().
    // This avoids the vertical-alignment issue that occurs when Charge & Remain Time rows are not shown.
    Qt::Alignment rowHeadAlign = Qt::AlignCenter | Qt::AlignBaseline;
    Qt::Alignment valColAlign = Qt::AlignCenter | Qt::AlignBaseline;
    Qt::Alignment unitColAlign = Qt::AlignCenter | Qt::AlignBaseline;

    if (chargeMode == charger::ChargeMode::UserSelect) {
        if(charger::GetChargeTimeShow() != 0)
        {
            setGridLabel(bookedTimeRhLabel, tr("Charging Time:"), rowHeadAlign);
            chargeDetailsGrid.addWidget(&bookedTimeRhLabel, 0, 0, Qt::AlignRight);

            setGridLabel(chargeHourUnitLabel, tr("hr"), unitColAlign);
            chargeDetailsGrid.addWidget(&chargeHourUnitLabel, 0, 2);

            setGridLabel(chargeMinuteUnitLabel, tr("min"), unitColAlign);
            chargeDetailsGrid.addWidget(&chargeMinuteUnitLabel, 0, 4);

            setGridLabel(chargeHourValLabel, "00", valColAlign);
            chargeDetailsGrid.addWidget(&chargeHourValLabel, 0, 1);

            setGridLabel(chargeMinuteValLabel, "00", valColAlign);
            chargeDetailsGrid.addWidget(&chargeMinuteValLabel, 0, 3);
        }
        if(charger::GetReaminTimeShow() == true)
        {
            setGridLabel(remainTimeRhLabel, tr("Remaining Time:"), rowHeadAlign);
            chargeDetailsGrid.addWidget(&remainTimeRhLabel, 1, 0, Qt::AlignRight);

            setGridLabel(remainHourUnitLabel, tr("hr"), unitColAlign);
            chargeDetailsGrid.addWidget(&remainHourUnitLabel, 1, 2);

            setGridLabel(remainMinuteUnitLabel, tr("min"), unitColAlign);
            chargeDetailsGrid.addWidget(&remainMinuteUnitLabel, 1, 4);

            setGridLabel(remainHourValLabel, "00", valColAlign);
            chargeDetailsGrid.addWidget(&remainHourValLabel, 1, 1);

            setGridLabel(remainMinuteValLabel, "00", valColAlign);
            chargeDetailsGrid.addWidget(&remainMinuteValLabel, 1, 3);
        }
    }

    int meterValStartRow = (chargeMode == charger::ChargeMode::UserSelect)? 2 : 0;
    if(charger::GetVoltageShow() == true)
    {
        setGridLabel(voltageRhLabel, tr("Voltage:"), rowHeadAlign);
        chargeDetailsGrid.addWidget(&voltageRhLabel, meterValStartRow + 0, 0, Qt::AlignRight);
    }
    if(charger::GetCurrentShow() == true)
    {
        setGridLabel(currentRhLabel, tr("Current:"), rowHeadAlign);
        chargeDetailsGrid.addWidget(&currentRhLabel, meterValStartRow + 1, 0, Qt::AlignRight);
    }
    if(charger::GetEnergyShow() == true)
    {
        setGridLabel(energyRhLabel, tr("Energy:"), rowHeadAlign);
        chargeDetailsGrid.addWidget(&energyRhLabel, meterValStartRow + 2, 0, Qt::AlignRight);
    }

    for (int i = 0; i < phases; i++) {
        if(charger::GetVoltageShow() == true)
        {
            setGridLabel(voltageValLabel[i], makeDisplayTextOfPhaseValues(0.0, i, 0, phases), valColAlign);
            chargeDetailsGrid.addWidget(&(voltageValLabel[i]), meterValStartRow + 0, 1 + i);
        }
        if(charger::GetCurrentShow() == true)
        {
            setGridLabel(currentValLabel[i], makeDisplayTextOfPhaseValues(0.0, i, 1, phases), valColAlign);
            chargeDetailsGrid.addWidget(&(currentValLabel[i]), meterValStartRow + 1, 1 + i);
        }
    }
    // Note: the energy value span across columns if phases > 1
    if(charger::GetEnergyShow() == true)
    {
        setGridLabel(energyValLabel, "0.0", valColAlign);
        chargeDetailsGrid.addWidget(&energyValLabel, meterValStartRow +2, 1, 1, phases);
    }


    int unitColOffset = (phases == 3)? 2 : 0;
    if(charger::GetVoltageShow() == true)
    {
        setGridLabel(voltageUnitLabel, tr("V"), unitColAlign);
        chargeDetailsGrid.addWidget(&voltageUnitLabel, meterValStartRow + 0, 2 + unitColOffset);
    }
    if(charger::GetCurrentShow() == true)
    {
        setGridLabel(currentUnitLabel, tr("A"), unitColAlign);
        chargeDetailsGrid.addWidget(&currentUnitLabel, meterValStartRow + 1, 2 + unitColOffset);
    }
    if(charger::GetEnergyShow() == true)
    {
        setGridLabel(energyUnitLabel, tr("kWh"), unitColAlign);
        chargeDetailsGrid.addWidget(&energyUnitLabel, meterValStartRow + 2, 2 + unitColOffset);
    }

}

void ChargingWindow::prepareStandaloneModeAuthWin()
{
    qWarning() << "CHARGING WINDOW: Prepare standalone mode auth win";
    if(charger::getRfidSupported())
    {
        qWarning() << "CHARGING WINDOW: Prepare standalone mode auth win. RFID";
        // Instantiate a RFID Window (without showing to user) to capture the RFID reader.
        // - Allows the user to stop charging simply by holding the NFC next to the charger,
        //  without the need to press the 'Quit' button.
        standaloneModeAuthWin = new RFIDWindow(this);
        standaloneModeAuthWin->setWindowTitle("STOP");    //未用
    }
    else
    {
        qWarning() << "CHARGING WINDOW: Prepare standalone mode auth win. Password";
        ChargeRequestPassword *reqPassword = new ChargeRequestPassword();
        // reqPassword will de-allocate itself when the there is no more work to be done
        // (lack of reference to reqPassword object)
        standaloneModeAuthWin = reqPassword->Start(parentWidget());
    }
    standaloneModeAuthWin->setWindowState(charger::Platform::instance().defaultWinState);
    connect(standaloneModeAuthWin, &QMainWindow::destroyed, this, &ChargingWindow::onStandaloneModeAuthWinDestroyed);
}


void ChargingWindow::onRfidAuthAccepted()
{
    qWarning() << "CHARGING WINDOW: RFID auth accepted";
    if (waitRfidReleaseConn) {
        ui->statusLabel->setText(tr("Card Accepted. Please detach cable..."));
        if (charger::isConnLockEnabled())
            unlockConnectorAndSetTimer();
        rfidAuthorizer->stop();
    }else if(prompting4Suspend){
        rfidAuthorizer->stop();
        prompting4Suspend =false;
        pauseButton_execution();
        return;
    }
    else {
        // Stop charging
        m_ChargerA.StopCharge();
        if (charger::isConnLockEnabled())
            unlockConnectorAndSetTimer();
        // timer will monitor the subsequent change in charging-status.
        rfidAuthorizer->stop();
    }
}

void ChargingWindow::onRfidAuthCanceled()
{
    qWarning() << "CHARGING WINDOW: RFID auth canceled";
    if(prompting4Suspend){
        rfidAuthorizer->stop();
        prompting4Suspend =false;
        return;
    }
    // Do nothing and keep on charging?
    rfidAuthorizer->runBackground();
}

void ChargingWindow::updateButtonStates()
{
    bool enableQuitButton = false;
    if (stopChargingRequested) {
        enableQuitButton = false;
    }
    else {
        // Exit button and label
        ui->label_ExitBK->setStyleSheet(exitLabelStyle);
        ui->quitButton->setStyleSheet(exitButtonStyle);

        if(charger::isStandaloneMode()) {
            enableQuitButton = true;
        }
        else if (m_ChargerA.ConnType() == CCharger::ConnectorType::WallSocket)
        {
            enableQuitButton = true;
        }
        else
        {
            // Only allow user to stop-charging here if it is RFID (StartTransaction)
            // or while it is offline
            using namespace charger;
            switch(charger::getPayMethod()) {
            case charger::PayMethod::RFID:
                enableQuitButton = true;
                break;
            default:
                enableQuitButton = m_OcppClient.isOffline();
                break;
            }
            if(charger::GetSkipPayment())
            {
                enableQuitButton = true;
            }
        }
    }
    ui->label_ExitBK->setVisible(enableQuitButton);
    ui->quitButton->setVisible(enableQuitButton);

    if (charger::isHardKeySupportRequired()) {
        ui->quitButton->setCheckable(enableQuitButton);
        ui->quitButton->setChecked(enableQuitButton);
    }
    bool enablePauseButton = charger::GetEnablePauseCharging();
    if(charger::GetEnablePauseCharging()){
        ui->label_PauseBK->setStyleSheet(pauseLabelStyle);
        ui->pauseButton->setStyleSheet(pauseButtonStyle);
    }
    ui->pauseButton->setVisible(enablePauseButton);
    ui->label_PauseBK->setVisible(enablePauseButton);
}

bool ChargingWindow::debounceMinCurrent()
{
    qWarning() << "CHARGING WINDOW: Debounce min current";
    bool debounced = false;
    // only care if it is actively charging
    if (m_ChargerA.IsCharging() && !stopChargingRequested) {
        // Condition: all 3 phases are below threshold
        EvChargingDetailMeterVal meterVal = m_ChargerA.GetMeterVal();
        bool belowThreshold = (static_cast<int>(meterVal.I[0]) < charger::getMinCutoffChargeCurrent())
                && (static_cast<int>(meterVal.I[1]) < charger::getMinCutoffChargeCurrent())
                && (static_cast<int>(meterVal.I[2]) < charger::getMinCutoffChargeCurrent());
        if (belowThreshold)
            minCurrentDebounceCnt++;
        else
            minCurrentDebounceCnt = 0; // reset
        debounced = (minCurrentDebounceCnt >= charger::getCutoffDebouceTime());
    }

    return debounced;
}

void ChargingWindow::on_quitButton_clicked()
{
    isSuspended = 0;
    qWarning() << "CHARGING WINDOW: Quit button clicked";
    using namespace charger;
    ChargeMode cMode = getChargingMode();

    switch (cMode) {
    case ChargeMode::NoAuthChargeToFull:
        qWarning() << "CHARGING WINDOW: Quit button clicked. No auth charge to full->stop charge and unlock";
        Q_ASSERT(charger::isStandaloneMode());
        m_ChargerA.StopCharge();
        if (isConnLockEnabled())
            unlockConnectorAndSetTimer();
        break;
    default:
        if (charger::isStandaloneMode()) { // Standalone-Mode
            qWarning() << "CHARGING WINDOW: Quit button clicked. Standalone mode. Show standalone mode auth win";
            if (!standaloneModeAuthWin)
                prepareStandaloneModeAuthWin();
            standaloneModeAuthWin->setWindowState(Platform::instance().defaultWinState);
            standaloneModeAuthWin->show();
        }
        else {
            qWarning() << "CHARGING WINDOW: Quit button clicked. Online mode";
            PayMethod pay = getPayMethod();
            switch (pay) {
            case PayMethod::RFID:
                qWarning() << "CHARGING WINDOW: Quit button clicked. Online mode. RFID auth";
                rfidAuthorizer->start(ChargerRequestRfid::Purpose::EndCharge);
                break;
            default:
                if (m_OcppClient.isOffline()) {
                    qWarning() << "CHARGING WINDOW: Quit button clicked. Online mode. Offline->stop charge and unlock";
                    // Allow user to stop charging and leave if it is currently offline.
                    qWarning() << "Ocpp CS is not connected. Allow user to stop charging with quit-button.";
                    m_ChargerA.StopCharge();
                    if (isConnLockEnabled())
                        unlockConnectorAndSetTimer();
                }
                else if (m_ChargerA.ConnType() == CCharger::ConnectorType::WallSocket)
                {
                    m_ChargerA.StopCharge();
                    if (isConnLockEnabled())
                        unlockConnectorAndSetTimer();
                }
                else {
                    if(charger::GetSkipPayment())
                    {
                        m_ChargerA.StopCharge();
                        if (isConnLockEnabled())
                            unlockConnectorAndSetTimer();
                        qWarning() << "Quit button click at ChargingWindow accept. Skip payment method.";
                    }
                    else
                    {
                        qWarning() << "Quit button click at ChargingWindow Ignored. Payment mode is not RFID.";
                    }
                }
                break;
            }
        }
        break;
    }

    //test: XXX
    //m_ChargerA.TransactionId =-1;
    //m_ChargerA.State &=0xfffffffe;
    //m_ChargerA.Status =ocpp::ChargerStatus::Finishing;
}

void ChargingWindow::onChargeMonitorTimerEvent()
{
    if(charger::GetChargeTimeShow() == 2)
    {
        updateTimeLabel(chargeHourValLabel, chargeMinuteValLabel, m_ChargerA.ChargeTimeValue - m_ChargerA.RemainTime - _C_SECOND_OFFSET);
    }

    if(charger::GetReaminTimeShow() == true)
    {
        if(charger::GetQueueingMode())
        {
            if(m_ChargerA.queueingWaiting == false)
            {
                updateTimeLabel(remainHourValLabel, remainMinuteValLabel, (m_ChargerA.queueingCountRemainingTime->remainingTime())/1000);
            }
        }
        else
        {
            updateTimeLabel(remainHourValLabel, remainMinuteValLabel, m_ChargerA.RemainTime);
        }
    }

    //ui->label_V->setText(QString::number(m_ChargerA.ChargerNewRaw.Va/100.0, 'f', 1));
    EvChargingDetailMeterVal meterVal = m_ChargerA.GetMeterVal();
    int numPhases = forceSinglePhaseDisplay? 1 : charger::getMeterPhase();
    for (int i = 0; i < numPhases; i++) {
        QString vStr = makeDisplayTextOfPhaseValues(meterVal.V[i] / 100.0, i, 0, numPhases);
        QString iStr;
        if(meterVal.I[i] / 1000.0 >= 10)
        {
            iStr = makeDisplayTextOfPhaseValues(meterVal.I[i] / 1000.0, i, 0, numPhases);
        }
        else
        {
            iStr = makeDisplayTextOfPhaseValues(meterVal.I[i] / 1000.0, i, 1, numPhases);
        }

        if(charger::GetVoltageShow() == true)
        {
            voltageValLabel[i].setText(vStr);
        }
        if(charger::GetCurrentShow() == true)
        {
            currentValLabel[i].setText(iStr);
        }
    }

    double chargedAmt = CCharger::meterValSubtract(m_ChargerA.MeterStart, meterVal.Q) / 10.0;
    if(charger::GetEnergyShow() == true)
    {
        energyValLabel.setText(QString::number(chargedAmt, 'f', CCharger::MeterDecPts));
    }

    bool wasSafeModeLabelVisible = ui->label_SafeMode->isVisible();
    if(!charger::isStandaloneMode() && charger::useSafeModeCurrentWhenOffline() && m_OcppClient.isOffline())
        ui->label_SafeMode->setVisible(true);  //执行安全电流充电
    else
        ui->label_SafeMode->setVisible(false);
    if (wasSafeModeLabelVisible != ui->label_SafeMode->isVisible())
        updateButtonStates();

    //add load management message
    if(charger::getMaxCutoffChargeCurrent() > m_ChargerA.GetCurrentLimit() && !wasSafeModeLabelVisible && m_ChargerA.GetCurrentLimit() && charger::GetLoadManagementShow() != 0)
    {
        ui->loadmanagemsg->setStyleSheet(loadmanageStyle);
        ui->loadmanagemsg->setText(tr("This charger is under load management"));
    }
    else
    {
        ui->loadmanagemsg->setVisible(false);
    }

    //test data show:
//    ui->pushButton->setText(QString("%1\n%2-%3\n%4--%5").arg(m_ChargerA.RemainTime).arg(m_ChargerA.State,
//                            8,16,QLatin1Char('0')).arg((int)m_ChargerA.Command, 0, 16).arg(m_ChargerA.ChargerNewRaw.Va/100.0,0, 'f', 1).arg(
//                            (m_ChargerA.ChargerNewRaw.Ia+m_ChargerA.ChargerNewRaw.Ib+m_ChargerA.ChargerNewRaw.Ic)/1000.0, 0,'f',3).toUpper());

    //qDebug() <<"chargingwindow status:" <<m_ChargerA.Status;

    //if(!(m_ChargerA.Status == ocpp::ChargerStatus::Charging || m_ChargerA.Status == ocpp::ChargerStatus::SuspendedEV || m_ChargerA.Status == ocpp::ChargerStatus::SuspendedEVSE))

    if(m_ChargerA.Status ==ocpp::ChargerStatus::Finishing || m_ChargerA.OldStatus ==ocpp::ChargerStatus::Finishing)
    {
        m_ChargerA.StopCharge(); //stop counting when charge is stop by PCB
        qWarning() << "CHARGING WINDOW: OCPP finishing";
        // No need to wait for Lock-Status if Connector-Lock is not enabled
        if((!charger::isConnLockEnabled()) || m_ChargerA.IsUnLock())
        {
            qWarning() << "CHARGING WINDOW: OCPP finishing. Lock is disabled or unlocked. Close window now";
            chargeMonitorTimer.stop();

            pCurWin =new ChargeFinishWindow(ocppTransId, this->parentWidget());
            pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
            pCurWin->show();
            this->close();

            if (pMsgWin != nullptr && pMsgWin->windowTitle() == "P7.UNLOCK") {
                pMsgWin->close();
                pMsgWin = nullptr;
            }
        }
        else
        {
            qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking";
            qWarning() << "ChargerA is Finishing State but Connector remain Locked.";
            // Timed-Charging falls into this else-case.
            if (!charger::isStandaloneMode()) {
                // Both Remote (Kiosk/App) and Local RFID
                qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking. Online mode";
                if (!waitRfidReleaseConn) {
                    //show msg according to payment method
                    if(charger::getPayMethod() == charger::PayMethod::RFID) {
                        ui->statusLabel->setText(tr("Please present RFID card to release cable."));
                        waitRfidReleaseConn = true;
                    }
                    if(charger::getPayMethod() == charger::PayMethod::App) {
                        ui->statusLabel->setText(tr("Please use App to release cable."));
                    }
                    if(charger::getPayMethod() == charger::PayMethod::App_Octopus) {
                        ui->statusLabel->setText(tr("Please use App/Kiosk to release cable."));
                    }
                    if(charger::getPayMethod() == charger::PayMethod::Octopus) {
                        ui->statusLabel->setText(tr("Please use Kiosk to release cable."));
                    }
                    if(charger::GetSkipPayment())
                    {
                        ui->statusLabel->setText("");
                        unlockConnectorAndSetTimer();
                    }
                }
                // Exception to this case, unlock if it is offline
                if (m_OcppClient.isOffline()) {
                    qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking. Online mode. Offline now, unlock";
                    qWarning() << "Charger is currently offline, unlock immediately.";
                    if (charger::isConnLockEnabled())
                        unlockConnectorAndSetTimer();
                }
                if (m_ChargerA.GetErrorCode() != ocpp::ChargePointErrorCode::NoError) {
                    qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking. Online mode. Get error, unlock";
                    qWarning() << "Charger faulted, unlock immediately.";
                    if (charger::isConnLockEnabled())
                        unlockConnectorAndSetTimer();
                }
            }
            else {
                qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking. Standalone mode. Unlock";
                if (charger::isConnLockEnabled())
                    unlockConnectorAndSetTimer();   //充完即开锁, 防止网络故障, 车无法开走
            }
        }
    }
    else {
        qWarning() << "CHARGING WINDOW: OCPP not finishing";
        EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
         if(!charger::isStandaloneMode() && reg.hasActiveTransaction()) {
             if (reg.activeTransactionHasTransId()) {
                 ocppTransId = reg.getActiveTransactionId();
             }
         }
         if (m_ChargerA.GetErrorCode() != ocpp::ChargePointErrorCode::NoError) {
             qWarning() << "CHARGING WINDOW: OCPP not finishing. Still locking. Online mode. Get error, unlock";
             qWarning() << "Charger faulted, unlock immediately.";
             if (charger::isConnLockEnabled())
                 unlockConnectorAndSetTimer();
             pCurWin =new ChargeFinishWindow(ocppTransId, this->parentWidget());
             pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
             pCurWin->show();
             this->close();
         }
    }

    // Only apply minimum-charging-current setting to Standalone-Mode
    if ((charger::isStandaloneMode() || charger::GetLowCurrentStop()) && debounceMinCurrent()) {
        qWarning() << "Charging current drops below minimum (CurrentStop). Stop Charging now.";
        // Do not care about the Authorization because it is driven by RFID?
        m_ChargerA.StopCharge();
        if (charger::isConnLockEnabled())
            unlockConnectorAndSetTimer();
    }

    if (m_ChargerA.IsChargingOrSuspended()) {
        qWarning() << "CHARGING WINDOW: OCPP charging or suspended";
        if(m_SaveTime++ >15)
        {
            m_SaveTime =0;
            m_ChargerA.ModifyRecord();
        }
    }
}

void ChargingWindow::on_pauseButton_clicked(){
    if(!charger::NeedRFIDCheckBeforeSuspend()){
        pauseButton_execution();
    }else{
        prompting4Suspend = true;
        charger::PayMethod pay = charger::getPayMethod();
        if(pay!=charger::PayMethod::RFID){
            //Note: the WaitingWindow::rfidAuthorizer could be holding the
            // RFID-Card-Reader Serial-Port. But since that thread will
            // keep retrying on open-failures, this rfidAuthorizer will
            // eventually be able to open the RFID-reader once WaitingWindow
            // closed.
            rfidAuthorizer = std::make_unique<ChargerRequestRfid>(this);
            connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestAccepted, this, &ChargingWindow::onRfidAuthAccepted);
            connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestCanceled, this, &ChargingWindow::onRfidAuthCanceled);
        }
        rfidAuthorizer->setStartTxRfid(m_ChargerA.IdTag);
        rfidAuthorizer->start(ChargerRequestRfid::Purpose::SuspendCharge, true); // listen to RFID card in the background
    }
}

void ChargingWindow::pauseButton_execution(){
    //suspend EVSE
    if(chargeMonitorTimer.isActive()){
        chargeMonitorTimer.stop();
    }

    //stop charging
    m_ChargerA.ChargerIdle();

    qWarning() << "CHARGING WINDOW: Keep Locked"<<charger::GetKeepLockedWhenSuspended();
    //unlock connector
    if (!m_ChargerA.IsUnLock() && !charger::GetKeepLockedWhenSuspended())
        m_ChargerA.UnLock();

    suspendWindow = new SuspendChargingWindow(this);
    connect(suspendWindow, &SuspendChargingWindow::finishSuspension,this,&ChargingWindow::onResumeFromSuspension);
    suspendWindow->setWindowState(charger::Platform::instance().defaultWinState);
    suspendWindow->setWindowTitle(QString("P7.SUSPEND"));
    EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
    if(!charger::isStandaloneMode() && reg.hasActiveTransaction()) {
     if (reg.activeTransactionHasTransId()) {
         suspendWindow->transId = reg.getActiveTransactionId();
     }
    }
    suspendWindow->show();
    pCurWin = suspendWindow;
    qWarning() << "CHARGING WINDOW: suspendWindow on";
    isSuspended = 1;

    qWarning() << "CHARGING WINDOW count:"<<instanceCount;
    qWarning() << "suspend WINDOW count:"<<SuspendChargingWindow::instanceCount;
}

void ChargingWindow::keyPressEvent(QKeyEvent *event)
{
    //qWarning() << "CHARGING WINDOW: Key press event";
    if(event->key() == Qt::Key_Return)
    {
        if (ui->quitButton->isVisible() && ui->quitButton->isEnabled() && ui->quitButton->isChecked())
            on_quitButton_clicked();
        else if (ui->pauseButton->isVisible() && ui->pauseButton->isEnabled() && ui->pauseButton->isChecked()){
            on_pauseButton_clicked();
            //qWarning() << "CHARGING WINDOW: pause button triggered";
        }
    }else if(event->key() ==Qt::Key_Right)
    {
        ui->pauseButton->setCheckable(true);
        if(ui->pauseButton->isChecked())
        {
            ui->pauseButton->setChecked(false);
            ui->quitButton->setChecked(true);
        }else //if(ui->quitButton->isChecked())
        {
            ui->quitButton->setChecked(false);
            ui->pauseButton->setChecked(true);
        }
    }
    // The following are development-purpose
    if(event->key() == Qt::Key_C)
    {
        m_ChargerA.setFakeCurrentOffset(5000);
    }
    if(event->key() == Qt::Key_D)
    {
        m_ChargerA.setFakeCurrentOffset(1000);
    }
    if (event->key() == Qt::Key_V) {
        ui->chargeDetailsWidget->setVisible(!ui->chargeDetailsWidget->isVisible());
    }
}

void ChargingWindow::onChargerStopChargeRequested()
{
    qWarning() << "CHARGING WINDOW: Charger stop charge requested";
    if (!stopChargingRequested) {
        stopChargingRequested = true;

        ui->statusLabel->setText(tr("Charging Stops..."));
        updateButtonStates();
    }
}

void ChargingWindow::onStandaloneModeAuthWinDestroyed(QObject *)
{
    qWarning() << "CHARGING WINDOW: Standalone mode auth window destroyed";
    this->standaloneModeAuthWin = nullptr;
}

void ChargingWindow::on_unlockButton_clicked()
{
    qWarning() << "CHARGING WINDOW: Unlock button clicked";
    if (charger::isConnLockEnabled()) {
        if (!m_ChargerA.IsUnLock())
            unlockConnectorAndSetTimer();
    }
}

void ChargingWindow::unlockConnectorAndSetTimer()
{
    qWarning() << "CHARGING WINDOW: unlock connector and set timer";
    if (!m_ChargerA.IsUnLock())
        m_ChargerA.UnLock();
    if (!unlockTimer.isActive()) {
        unlockTimer.start();
        qDebug() << "Unlock Timer Started at " << QDateTime::currentDateTime().toString(Qt::DateFormat::RFC2822Date);
    }
}

void ChargingWindow::onUnlockTimerEvent()
{
    qWarning() << "CHARGING WINDOW: Unlock timer event, check unlock success or not";
    qDebug() << "Unlock Timer Timed out at " << QDateTime::currentDateTime().toString(Qt::DateFormat::RFC2822Date);
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

void ChargingWindow::onResumeFromSuspension(int res)
{
    pCurWin = this;
    delete suspendWindow;
    isSuspended = 0;
    if(res>0){
        //lock connector
        if (m_ChargerA.IsUnLock())
            m_ChargerA.Lock();

        //resume from suspend EVSE
        m_ChargerA.ResumeCharge();

        chargeMonitorTimer.start();
    }else{
        EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
        QString transId = reg.getActiveTransactionId();
        QDateTime stopDt = QDateTime::currentDateTime();
        m_OcppClient.addStopTransactionReq(transId, stopDt, m_ChargerA.ChargedQ, ocpp::Reason::Remote);

        emit m_ChargerA.chargeFinished(stopDt, m_ChargerA.ChargedQ, ocpp::Reason::Remote);
        //m_ChargerA.StopCharge();

        chargeMonitorTimer.start();
        quitOnResumeFromSuspension();
    }
}

void ChargingWindow::quitOnResumeFromSuspension(){
    if(charger::GetChargeTimeShow() == 2)
    {
        updateTimeLabel(chargeHourValLabel, chargeMinuteValLabel, m_ChargerA.ChargeTimeValue - m_ChargerA.RemainTime - _C_SECOND_OFFSET);
    }

    if(charger::GetReaminTimeShow() == true)
    {
        if(charger::GetQueueingMode())
        {
            if(m_ChargerA.queueingWaiting == false)
            {
                updateTimeLabel(remainHourValLabel, remainMinuteValLabel, (m_ChargerA.queueingCountRemainingTime->remainingTime())/1000);
            }
        }
        else
        {
            updateTimeLabel(remainHourValLabel, remainMinuteValLabel, m_ChargerA.RemainTime);
        }
    }

    //ui->label_V->setText(QString::number(m_ChargerA.ChargerNewRaw.Va/100.0, 'f', 1));
    EvChargingDetailMeterVal meterVal = m_ChargerA.GetMeterVal();
    int numPhases = forceSinglePhaseDisplay? 1 : charger::getMeterPhase();
    for (int i = 0; i < numPhases; i++) {
        QString vStr = makeDisplayTextOfPhaseValues(meterVal.V[i] / 100.0, i, 0, numPhases);
        QString iStr;
        if(meterVal.I[i] / 1000.0 >= 10)
        {
            iStr = makeDisplayTextOfPhaseValues(meterVal.I[i] / 1000.0, i, 0, numPhases);
        }
        else
        {
            iStr = makeDisplayTextOfPhaseValues(meterVal.I[i] / 1000.0, i, 1, numPhases);
        }

        if(charger::GetVoltageShow() == true)
        {
            voltageValLabel[i].setText(vStr);
        }
        if(charger::GetCurrentShow() == true)
        {
            currentValLabel[i].setText(iStr);
        }
    }

    double chargedAmt = CCharger::meterValSubtract(m_ChargerA.MeterStart, meterVal.Q) / 10.0;
    if(charger::GetEnergyShow() == true)
    {
        energyValLabel.setText(QString::number(chargedAmt, 'f', CCharger::MeterDecPts));
    }

    bool wasSafeModeLabelVisible = ui->label_SafeMode->isVisible();
    if(!charger::isStandaloneMode() && charger::useSafeModeCurrentWhenOffline() && m_OcppClient.isOffline())
        ui->label_SafeMode->setVisible(true);  //执行安全电流充电
    else
        ui->label_SafeMode->setVisible(false);
    if (wasSafeModeLabelVisible != ui->label_SafeMode->isVisible())
        updateButtonStates();

    //add load management message
    if(charger::getMaxCutoffChargeCurrent() > m_ChargerA.GetCurrentLimit() && !wasSafeModeLabelVisible && m_ChargerA.GetCurrentLimit() && charger::GetLoadManagementShow() != 0)
    {
        ui->loadmanagemsg->setStyleSheet(loadmanageStyle);
        ui->loadmanagemsg->setText(tr("This charger is under load management"));
    }
    else
    {
        ui->loadmanagemsg->setVisible(false);
    }

    m_ChargerA.StopCharge(); //stop counting when charge is stop by PCB
    qWarning() << "CHARGING WINDOW: OCPP finishing";
    // No need to wait for Lock-Status if Connector-Lock is not enabled
    if((!charger::isConnLockEnabled()) || m_ChargerA.IsUnLock())
    {
        qWarning() << "CHARGING WINDOW: OCPP finishing. Lock is disabled or unlocked. Close window now";
        chargeMonitorTimer.stop();

        pCurWin =new ChargeFinishWindow(ocppTransId, this->parentWidget());
        pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
        pCurWin->show();
        this->close();

        if (pMsgWin != nullptr && pMsgWin->windowTitle() == "P7.UNLOCK") {
            pMsgWin->close();
            pMsgWin = nullptr;
        }
    }
    else
    {
        qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking";
        qWarning() << "ChargerA is Finishing State but Connector remain Locked.";
        // Timed-Charging falls into this else-case.
        if (!charger::isStandaloneMode()) {
            // Both Remote (Kiosk/App) and Local RFID
            qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking. Online mode";
            if (!waitRfidReleaseConn) {
                //show msg according to payment method
                if(charger::getPayMethod() == charger::PayMethod::RFID) {
                    ui->statusLabel->setText(tr("Please present RFID card to release cable."));
                    waitRfidReleaseConn = true;
                }
                if(charger::getPayMethod() == charger::PayMethod::App) {
                    ui->statusLabel->setText(tr("Please use App to release cable."));
                }
                if(charger::getPayMethod() == charger::PayMethod::App_Octopus) {
                    ui->statusLabel->setText(tr("Please use App/Kiosk to release cable."));
                }
                if(charger::getPayMethod() == charger::PayMethod::Octopus) {
                    ui->statusLabel->setText(tr("Please use Kiosk to release cable."));
                }
                if(charger::GetSkipPayment())
                {
                    ui->statusLabel->setText("");
                    unlockConnectorAndSetTimer();
                }
            }
            // Exception to this case, unlock if it is offline
            if (m_OcppClient.isOffline()) {
                qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking. Online mode. Offline now, unlock";
                qWarning() << "Charger is currently offline, unlock immediately.";
                if (charger::isConnLockEnabled())
                    unlockConnectorAndSetTimer();
            }
            if (m_ChargerA.GetErrorCode() != ocpp::ChargePointErrorCode::NoError) {
                qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking. Online mode. Get error, unlock";
                qWarning() << "Charger faulted, unlock immediately.";
                if (charger::isConnLockEnabled())
                    unlockConnectorAndSetTimer();
            }
        }
        else {
            qWarning() << "CHARGING WINDOW: OCPP finishing. Still locking. Standalone mode. Unlock";
            if (charger::isConnLockEnabled())
                unlockConnectorAndSetTimer();   //充完即开锁, 防止网络故障, 车无法开走
        }
    }
}

/* for QT Test Purpose*/
unsigned int ChargingWindow::getObjectNum(){
    return this->objectNum;
}

unsigned int ChargingWindow::getInstanceCount(){
    return ChargingWindow::instanceCount;
}

ChargingWindow * ChargingWindow::getInstanceFromPool(int at){
    return ChargingWindow::objectPool.at(at);
}

unsigned int ChargingWindow::instanceCount = 0;
QList<ChargingWindow *> ChargingWindow::objectPool = QList<ChargingWindow *>();
