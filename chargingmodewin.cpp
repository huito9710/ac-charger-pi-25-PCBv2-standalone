#include "chargingmodewin.h"
#include "ui_chargingmodewin.h"
#include "setchargingtimewin.h"
#include "setdelaychargingwin.h"
#include "waittingwindow.h"
#include "api_function.h"
#include "gvar.h"
#include "hostplatform.h"

ChargingModeWin::ChargingModeWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChargingModeWin),
    winBgStyle(QString("QWidget#centralwidget {background-image: url(%1chargingModeWin.%2.jpg);}")
               .arg(charger::Platform::instance().guiImageFileBaseDir)
               .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "EN" : "ZH")),
    chargeTimeWin(nullptr)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    switch (m_ChargerA.ConnType()) {
    case CCharger::ConnectorType::EvConnector:
    case CCharger::ConnectorType::AC63A_NoLock:
        connect(&connMonitorTimer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
        connMonitorTimer.start(500);
        break;
    default:
        break;
    }

    this->setStyleSheet(winBgStyle);

    // Overrides the unix-paths defined in pushbuttons
    QString exitButtonStyleSheet = QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1/PNG/Rect2.png);}").arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->pushButton_exit->setStyleSheet(exitButtonStyleSheet);
    QString modeButtonStyleSheet = QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1/PNG/Rect3.png);}").arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->pushButton_auto->setStyleSheet(modeButtonStyleSheet);
    ui->pushButton_time->setStyleSheet(modeButtonStyleSheet);
    ui->pushButton_delay->setStyleSheet(modeButtonStyleSheet);

    if(charger::isHardKeySupportRequired())
    {
        ui->pushButton_auto->setCheckable(true);
        ui->pushButton_delay->setCheckable(true);
        ui->pushButton_time->setCheckable(true);
        ui->pushButton_exit->setCheckable(true);

        ui->pushButton_auto->setChecked(true);
    }
}

ChargingModeWin::~ChargingModeWin()
{
    delete ui;

    if(connMonitorTimer.isActive())
        connMonitorTimer.stop();
    if (chargeTimeWin != nullptr)
        chargeTimeWin->close();
}

void ChargingModeWin::on_pushButton_auto_clicked()
{
    m_ChargerA.StartChargeToFull();

    pCurWin =new WaittingWindow(this->parentWidget());
    pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
    pCurWin->show();
    this->close();
    qDebug() <<"Auto in Mode";
}

void ChargingModeWin::on_pushButton_time_clicked()
{
    chargeTimeWin =new SetChargingTimeWin(this);
    connect(chargeTimeWin, &SetChargingTimeWin::chargeTimeSet, this, &ChargingModeWin::onChargeTimeSet);
    chargeTimeWin->setWindowState(charger::Platform::instance().defaultWinState);
    chargeTimeWin->show();
    // leave pCurWin alone, keep this ChargingModeWin open in the back
}

void ChargingModeWin::on_pushButton_delay_clicked()
{
    pCurWin =new SetDelayChargingWin(this->parentWidget());
    pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
    pCurWin->show();
    this->close();
}

void ChargingModeWin::on_pushButton_exit_clicked()
{
    m_ChargerA.UnLock();
    m_ChargerA.ChargerIdle();
    this->close();
    pCurWin =NULL;
}

void ChargingModeWin::on_TimerEvent()
{
    if(!(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected()))
    {
        if((m_ChargerA.Command &_C_CHARGEMASK) !=_C_CHARGECOMMAND)
            on_pushButton_exit_clicked();
    }
}

void ChargingModeWin::onChargeTimeSet(bool completed)
{
    chargeTimeWin->close();
    chargeTimeWin = nullptr;
    if (completed) {
        // show Waiting Window
         pCurWin = new WaittingWindow(this->parentWidget());
         pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
         pCurWin->show();
         this->close();
         m_ChargerA.StartCharge();
    }
    // on cancel, user will see this mode-selection screen again.
}

void ChargingModeWin::keyPressEvent(QKeyEvent *event)
{
    if(event->key() ==Qt::Key_Right)
    {
        if(ui->pushButton_auto->isChecked())
        {
            ui->pushButton_auto->setChecked(false);
            ui->pushButton_time->setChecked(true);
        }
        else if(ui->pushButton_time->isChecked())
        {
            ui->pushButton_time->setChecked(false);
            ui->pushButton_delay->setChecked(true);
        }
        else if(ui->pushButton_delay->isChecked())
        {
            ui->pushButton_delay->setChecked(false);
            ui->pushButton_exit->setChecked(true);
        }
        else if(ui->pushButton_exit->isChecked())
        {
            ui->pushButton_exit->setChecked(false);
            ui->pushButton_auto->setChecked(true);
        }
    }
    else if(event->key() ==Qt::Key_Return)
    {
        if(ui->pushButton_auto->isChecked())
            on_pushButton_auto_clicked();
        else if(ui->pushButton_delay->isChecked())
            on_pushButton_delay_clicked();
        else if(ui->pushButton_exit->isChecked())
            on_pushButton_exit_clicked();
        else if(ui->pushButton_time->isChecked())
            on_pushButton_time_clicked();
    }
}
