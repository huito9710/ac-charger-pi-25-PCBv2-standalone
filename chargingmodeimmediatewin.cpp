#include "chargingmodeimmediatewin.h"
#include "ui_chargingmodewin.h"
#include "hostplatform.h"
#include "gvar.h"
#include "waittingwindow.h"

ChargingModeImmediateWin::ChargingModeImmediateWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChargingModeWin),
    winBgStyle(QString("QWidget#centralwidget { background-image: url(%1chargingModeImmedWin%3.%2.jpg);}")
               .arg(charger::Platform::instance().guiImageFileBaseDir)
               .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "EN":"ZH")
               .arg(charger::GetNoChargeToFull()? "NoFull":"")),
    chargeTimeWin(nullptr),
    connIcon(m_OcppClient)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);
    // Disable the 'DelayCharging Button'
    ui->pushButton_delay->setEnabled(false);

    this->setStyleSheet(winBgStyle);

    switch (m_ChargerA.ConnType()) {
    case CCharger::ConnectorType::EvConnector:
    case CCharger::ConnectorType::AC63A_NoLock:
        connect(&cableMonitorTimer, &QTimer::timeout, this, &ChargingModeImmediateWin::onCableMonitorTimeout);
        cableMonitorTimer.start(cableMonitorInterval);
        break;
    default:
        break;
    }

    // Checked Cursor Style
    QString menuButtonStyle = QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1/PNG/Rect3.png);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir);
    if(!charger::GetNoChargeToFull())
    {
        ui->pushButton_auto->setStyleSheet(menuButtonStyle);
    }
    ui->pushButton_time->setStyleSheet(menuButtonStyle);
    QString quitButtonStyle = QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1/PNG/Rect5.png);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->pushButton_exit->setStyleSheet(quitButtonStyle);

    if(charger::isHardKeySupportRequired())
    {
        // Default with 'Auto' (First choice)
        ui->pushButton_exit->setCheckable(true);
        ui->pushButton_exit->setChecked(false);
        if(charger::GetNoChargeToFull())
        {
            ui->pushButton_auto->setCheckable(false);
            ui->pushButton_auto->setChecked(false);
        } else {
            ui->pushButton_auto->setCheckable(true);
            ui->pushButton_auto->setChecked(true);
        }
        ui->pushButton_time->setCheckable(true);
        if(charger::GetNoChargeToFull())
        {
            ui->pushButton_time->setChecked(true);
        } else {
            ui->pushButton_time->setChecked(false);
        }
    }

    if(charger::GetNoChargeToFull())
    {
        ui->pushButton_auto->setVisible(false);
    }
}

ChargingModeImmediateWin::~ChargingModeImmediateWin()
{
    delete ui;

    if(cableMonitorTimer.isActive())
        cableMonitorTimer.stop();
}

// Cable Connection Monitoring
void ChargingModeImmediateWin::onCableMonitorTimeout()
{
    if(!(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected()))
    {
        if((m_ChargerA.Command &_C_CHARGEMASK) !=_C_CHARGECOMMAND)
            on_pushButton_exit_clicked();
    }
}

// Buttons Callbacks
void ChargingModeImmediateWin::on_pushButton_exit_clicked()
{
    m_ChargerA.UnLock();
    m_ChargerA.ChargerIdle();
    this->close();
    pCurWin =NULL;
}

void ChargingModeImmediateWin::on_pushButton_auto_clicked()
{
    // Start Charge-to-Full right-away
    m_ChargerA.StartChargeToFull();

    pCurWin =new WaittingWindow(this->parentWidget());
    pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
    pCurWin->show();
    this->close();
    qDebug() <<"Auto in Mode";
}

void ChargingModeImmediateWin::on_pushButton_time_clicked()
{
    chargeTimeWin  =new SetChargingTimeWin(this);
    connect(chargeTimeWin, &SetChargingTimeWin::chargeTimeSet, this, &ChargingModeImmediateWin::onChargeTimeSet);
    chargeTimeWin->setWindowState(charger::Platform::instance().defaultWinState);
    chargeTimeWin->show();
    // leave pCurWin alone, keep this ChargingModeWin open in the back
}

void ChargingModeImmediateWin::onChargeTimeSet(bool completed)
{
    chargeTimeWin->close();
    chargeTimeWin = nullptr;
    if (completed) {
        // show Waiting Window, *** assuming this requires Central-Server approval ***
         WaittingWindow *waitWin = new WaittingWindow(this->parentWidget());
         waitWin->setSeekApprovalBeforeCharge(false);
         pCurWin = waitWin;
         pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
         pCurWin->show();
         this->close();
         m_ChargerA.StartCharge();
    }
    // on cancel, user will see this mode-selection screen again.
}


void ChargingModeImmediateWin::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
    case Qt::Key_Right:
    case Qt::Key_PageDown:
        // exit -> auto -> time
        if(ui->pushButton_exit->isChecked())
        {
            ui->pushButton_exit->setChecked(false);
            if(charger::GetNoChargeToFull())
            {
                ui->pushButton_time->setChecked(true);
            } else {
                ui->pushButton_auto->setChecked(true);
            }
        }
        else if(ui->pushButton_auto->isChecked())
        {
            ui->pushButton_auto->setChecked(false);
            ui->pushButton_time->setChecked(true);
        }
        else if(ui->pushButton_time->isChecked())
        {
            ui->pushButton_time->setChecked(false);
            ui->pushButton_exit->setChecked(true);
        }
        else {
            // neither buttons are currently selected
            qWarning() << "ChargingModeImmediateWin(): no buttons are currently Checked, moving cursor to Auto";
            ui->pushButton_auto->setChecked(true);
        }
        break;
    case Qt::Key_Return:
        //qDebug()<<event->text()<<"="<<event->isAutoRepeat()<<"--"<<event->count();
        if(ui->pushButton_exit->isChecked())
            on_pushButton_exit_clicked();
        else if(ui->pushButton_auto->isChecked())
            on_pushButton_auto_clicked();
        else if(ui->pushButton_time->isChecked())
            on_pushButton_time_clicked();
        else
            Q_ASSERT(false);
        break;
    default:
        qWarning() << "Unhandled key event (" << event->key() << ").";
        break;
    }
}
