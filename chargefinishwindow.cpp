#include "chargefinishwindow.h"
#include "ui_chargefinishwindow.h"
#include "api_function.h"
#include "ccharger.h"
#include <QDateTime>
#include <QtGui>
#include <QWidget>
#include "const.h"
#include "gvar.h"
#include "hostplatform.h"
#include "statusledctrl.h"
#include "paymentmethodwindow.h"

ChargeFinishWindow::ChargeFinishWindow(QString transId, QWidget *parent) :
    QMainWindow(parent),
    cableCheckTimerInterval(200),
    defaultMinDisplayTime(4000),
    txnCheckTimerInterval(10000),
    ui(new Ui::ChargeFinishWindow),
    connIcon(m_OcppClient),
    displayedTime(0),
    ocppTransId(transId),
    ocppCsNotResponding(false),
    txnPendingToCs(false),
    paymentWinRunning(false)
{
    qWarning() << "CHARGING FINISH WINDOW: init";
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);

    connect(&cableCheckTimer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
    cableCheckTimer.start(cableCheckTimerInterval);
    connect(&txnCheckTimer, &QTimer::timeout, this, &ChargeFinishWindow::onCheckOcppResponseTimeout);
    txnCheckTimer.start(txnCheckTimerInterval);

    if (!(ocppTransId.isNull() || ocppTransId.isEmpty())) {
        updateStyleStrs(ChargeSummary::byEnergy, charger::getCurrDisplayLang());
        this->setStyleSheet(winBgStyle);
    }
    else {
        updateStyleStrs(ChargeSummary::byTimeWithConnFailure, charger::getCurrDisplayLang());
        this->setStyleSheet(winBgStyle);
    }

    QString quitButtonStyle = QString("QPushButton{background:transparent; border-image:url(%1PNG/quit.%2.png); image:none;} QPushButton:checked{image: url(%1PNG/Rect2.png);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir)
            .arg(charger::getCurrDisplayLang() == charger::DisplayLang::Chinese? "ZH" : "EN");
    ui->pushButton->setStyleSheet(quitButtonStyle);
    if(charger::isStandaloneMode()) //Not NetVer
    {
        setQuitButtonEnable(true);
        ui->pushButton->setText("");
    }
    else {
        setQuitButtonEnable(false); // wait until connector available and stop-transaction completed (if applicable?)
    }

    QString txt("#");
    GetSetting(QStringLiteral("[CHARGER NO]"), txt);
    //ui->label_No->setText(txt);
    ui->label_No->setText("");              //不使用
    ui->label_startdt->setVisible(false);   //不使用
    ui->label_enddt->setVisible(false);     //不使用

    // display Local time
    txt =m_ChargerA.StartDt.toLocalTime().toString("yyyy-MM-dd hh:mm");
    ui->label_startdt->setText(txt);

    QString plugCableLabelStyle = QString("qproperty-alignment: AlignCenter; color:white; font-family: Piboto, Calibri, sans-serif; font-size: 22px");
    ui->plugCableRemain->setStyleSheet(plugCableLabelStyle);
    ui->plugCableRemain->setText(tr("Remove cable to exit"));

    ulong second =m_ChargerA.ChargeTimeValue -m_ChargerA.RemainTime;
    if(charger::GetQueueingMode())
    {
        if(m_ChargerA.queueingWaiting == true)
        {
            second = 0;
        }
        else
        {
            if(m_ChargerA.chargedTime != 0)
            {
                second = m_ChargerA.chargedTime;
            }
            else
            {
                second = m_ChargerA.ChargeTimeValue;
            }
        }
    }

    int time =(second +59)/3600;
    ui->label_time->setText(QString("%1").arg(time, 2, 10, QLatin1Char('0')));
    time =(second +59 -time*3600)/60;
    ui->label_time_mm->setText(QString("%1").arg(time, 2, 10, QLatin1Char('0')));

    QDateTime dt =QDateTime::fromString(txt, "yyyy-MM-dd HH:mm:ss");
    dt =dt.addSecs(second);
    QDateTime curdt =QDateTime::currentDateTime();
    if(curdt >dt.addSecs(60)) dt =curdt;

    if (m_ChargerA.StopDt.isNull())
        txt = "Finish-Time Not available";
    else
        txt =m_ChargerA.StopDt.toLocalTime().toString("yyyy-MM-dd hh:mm");
    ui->label_enddt->setText(txt);

    double chargedAmt = CCharger::meterValSubtract(m_ChargerA.MeterStart, m_ChargerA.MeterStop) / 10.0;
    ui->label_Q->setText(QString::number(chargedAmt, 'f', CCharger::MeterDecPts));

    if (charger::isStandaloneMode()) {
        TransactionRecordFile recFile {QString(charger::getChargerNum().c_str()), charger::getMeterType()};
        m_ChargerA.SaveCurrentChargeRecord(recFile);
    }

}

void ChargeFinishWindow::setQuitButtonEnable(bool enableIt)
{
    ui->pushButton->setEnabled(enableIt);

    // only show if it is enabled
    ui->pushButton->setVisible(ui->pushButton->isEnabled()); // hide if it is not enabled

    if(charger::isHardKeySupportRequired()) {
        // only show cursor if it is enabled
        ui->pushButton->setCheckable(ui->pushButton->isEnabled());
        ui->pushButton->setChecked(ui->pushButton->isEnabled());
    }
}


ChargeFinishWindow::~ChargeFinishWindow()
{
    delete ui;
    if(cableCheckTimer.isActive())
        cableCheckTimer.stop();
//    if(charger::GetQueueingMode())
//    {
//        StatusLedCtrl::instance().setState(StatusLedCtrl::State::blue);
//    }
//    else
//    {
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::green);
//    }
}

void ChargeFinishWindow::updateStyleStrs(ChargeSummary chrSummary, charger::DisplayLang lang)
{
    using namespace charger;
    QString langInFn;
    switch (lang) {
    case DisplayLang::Chinese: langInFn = "ZH"; break;
    case DisplayLang::English: langInFn = "EN"; break;
    default:
        break;
    }
    QString fileBaseName;
    switch(chrSummary) {
    case ChargeSummary::byTime:
        fileBaseName = "P8"; break;
    case ChargeSummary::byTimeWithConnFailure:
        fileBaseName = "P8"; break;
    //case ChargeSummary::byTimeWithConnFailure: fileBaseName = "P8.2"; break;
    case ChargeSummary::byEnergy: fileBaseName = "chargingFinishWin"; break;
    }
    winBgStyle = QString("QWidget#centralwidget {background-image: url(%1%2.%3.jpg);}").arg(Platform::instance().guiImageFileBaseDir).arg(fileBaseName).arg(langInFn);
}

void ChargeFinishWindow::on_pushButton_clicked()
{ //HOME:
    qWarning() << "CHARGING FINISH WINDOW: Closing this window";
    checkStopPaymentWindow();
    m_ChargerA.UnLock();
    this->close();
    pCurWin =NULL;
}

void ChargeFinishWindow::checkStartPaymentWindow()
{
    //qWarning() << "CHARGING FINISH WINDOW: Check start payment window";
    // Open RFID Reader to allow user to start charging again at this screen.
    // Conditions:
    //  PaymentMethod is RFID
    //
    if (charger::getPayMethod() == charger::PayMethod::RFID) {
        if (!paymentWinRunning) {
            paymentWin = new PaymentMethodWindow(this->parentWidget());
            connect(paymentWin, &PaymentMethodWindow::bgWindowClosed, this, &ChargeFinishWindow::onBackgroundPaymentWindowClosed);
            paymentWin->runInBackground();
            paymentWinRunning = true;
        }
    }
}

void ChargeFinishWindow::checkStopPaymentWindow()
{
    qWarning() << "CHARGING FINISH WINDOW: Check stop payment window";
    // Open RFID Reader to allow user to start charging again at this screen.
    // Conditions:
    //  PaymentMethod is RFID
    //
    if (charger::getPayMethod() == charger::PayMethod::RFID) {
        if (paymentWinRunning) {
            if (paymentWin != nullptr) {
                disconnect(paymentWin, &PaymentMethodWindow::bgWindowClosed, nullptr, nullptr);
                paymentWin->close();
                paymentWin = nullptr;
            }
            paymentWinRunning = false;
        }
    }
}

void ChargeFinishWindow::on_TimerEvent()
{
    displayedTime += cableCheckTimerInterval;
    EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
    //m_ChargerA.TransactionId =-1;  //test no net
    if (!charger::isStandaloneMode()) {

        //qWarning() << "CHARGING FINISH WINDOW: Online mode";
        //if (reg.hasActiveTransaction())
            //qDebug () << "ChargerFinishWindow::onTimerEvent() Transaction-Id is " << reg.getActiveTransactionId();
        //else
            //qDebug () << "ChargerFinishWindow::onTimerEvent() there is no active transaction";
    }
    if(charger::isStandaloneMode() || !reg.hasActiveTransaction())
    {
        //qWarning() << "CHARGING FINISH WINDOW: Standalone mode";
        StatusLedCtrl::instance().clear();

        if(!m_ChargerA.StartDt.isNull())
        {
            m_ChargerA.StartDt = QDateTime(); // Clear Value
            m_ChargerA.ClearRecord();
        }

        if (m_ChargerA.Status == ocpp::ChargerStatus::Finishing)
        {
            //if((m_ChargerA.Command &_C_CHARGEMASK) !=_C_CHARGEIDLE)
            if (!m_ChargerA.IsIdleState())
                m_ChargerA.ChargerIdle();
            //setQuitButtonEnable(true);
            checkStartPaymentWindow();
        }
        else // connector is available
        {
            //if((!m_ChargerA.IsCablePlugIn()) &&(m_delay >15))   //15-使信息显示至少30"
            if((!m_ChargerA.IsCablePlugIn()) &&
                (charger::reduceScreenTransitions() || displayedTime >= defaultMinDisplayTime))    //2-使信息显示至少4"
            { //已拔线
                qWarning() << "CHARGING FINISH WINDOW: Cable plugged out";
                on_pushButton_clicked();
            }
        }
    }

    QThread::sleep(5);
    //qDebug() <<m_ChargerA.TransactionId <<"finish:"<<m_ChargerA.Status;
}


///
/// \brief ChargeFinishWindow::onCheckOcppResponseTimeout
/// 2 main cases
///     Transaction started offline: toggle the background window on whether EvTransactionRegistry is Empty.
///     Transaction started online: toggle the background window on whether THIS transaction-ID still present in registry
///
void ChargeFinishWindow::onCheckOcppResponseTimeout()
{
    setQuitButtonEnable(true);
    return;

    //qWarning() << "CHARGING FINISH WINDOW: Check OCPP response timeout";
    if (charger::isStandaloneMode())
        return;
    EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
    if(ocppTransId.isNull() || ocppTransId.isEmpty()) {
        if (!reg.hasActiveTransaction() && (reg.getNumNonActiveTransactions() == 0)) {
            // transactions still in registry
            if (txnPendingToCs) {
                updateStyleStrs(ChargeSummary::byEnergy, charger::getCurrDisplayLang());
                this->setStyleSheet(winBgStyle);
                txnPendingToCs = false;
            }
        }
        else {
            if (!txnPendingToCs) {
                updateStyleStrs(ChargeSummary::byTimeWithConnFailure, charger::getCurrDisplayLang());
                this->setStyleSheet(winBgStyle);
                txnPendingToCs = true;
            }
        }
    }
    else {
        if (reg.hasTransactionId(ocppTransId)) {
            if (!ocppCsNotResponding) {
                // ocppCsNotResponding: false -> true
                // Transaction-Id still in registry, implies the transaction has not been
                // fully reported to CS yet.
                updateStyleStrs(ChargeSummary::byTimeWithConnFailure, charger::getCurrDisplayLang());
                this->setStyleSheet(winBgStyle);
                StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashingred);
                ocppCsNotResponding = true;
                setQuitButtonEnable(true);
            }
        }
        else {
            if (ocppCsNotResponding) {
                // ocppCsNotResponding: true -> false
                // This case could happen if, the CS is offline at the beginning of Finishing,
                // but later on becomes online.
                ocppCsNotResponding = false; // reported to registry
                updateStyleStrs(ChargeSummary::byEnergy, charger::getCurrDisplayLang());
                this->setStyleSheet(winBgStyle);
            }
        }
    }
}

void ChargeFinishWindow::keyPressEvent(QKeyEvent *event)
{
    qWarning() << "CHARGING FINISH WINDOW: Key press event";
    if(event->key() ==Qt::Key_Return)
    {
        qDebug() <<"key exit";
        if(ui->pushButton->isEnabled()) on_pushButton_clicked();
    }
}


