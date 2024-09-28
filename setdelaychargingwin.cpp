#include "setdelaychargingwin.h"
#include "ui_setdelaychargingwin.h"
#include "api_function.h"
#include "gvar.h"
#include "delaychargingwin.h"
#include "hostplatform.h"
#include "chargerconfig.h"

SetDelayChargingWin::SetDelayChargingWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SetDelayChargingWin),
    winBgStyle(QString("QWidget#centralwidget {background-image: url(%1setDelayTimeWin.%2.jpg);}")
               .arg(charger::Platform::instance().guiImageFileBaseDir)
               .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "EN":"ZH"))
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

    Second =0;
    QString itemvalue="30";     //30'
    GetSetting(QStringLiteral("[INCDELAYTIME]"), itemvalue);
    dSecond =itemvalue.toULong() *60;

    // Overrides the unix-paths defined in pushbuttons
    QString exitButtonStyleSheet = QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1/PNG/Rect2.png);}").arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->pushButton_exit->setStyleSheet(exitButtonStyleSheet);
    QString timeButtonStyleSheet = QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1/PNG/Rect4.png);}").arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->pushButton_add->setStyleSheet(timeButtonStyleSheet);
    ui->pushButton_dec->setStyleSheet(timeButtonStyleSheet);
    ui->pushButton_ok->setStyleSheet(timeButtonStyleSheet);

    if(charger::isHardKeySupportRequired())
    {
        ui->pushButton_add->setCheckable(true);
        ui->pushButton_dec->setCheckable(true);
        ui->pushButton_ok->setCheckable(true);
        ui->pushButton_exit->setCheckable(true);

        ui->pushButton_add->setChecked(true);
    }
}

SetDelayChargingWin::~SetDelayChargingWin()
{
    delete ui;

    if(connMonitorTimer.isActive())
        connMonitorTimer.stop();
}

void SetDelayChargingWin::on_pushButton_add_clicked()
{
    QString maxtime ="1200";    //MAX 20h

    GetSetting(QStringLiteral("[MAXDELAYTIME]"), maxtime);

    if(Second +dSecond <=maxtime.toULong()*60)
    {
        Second +=dSecond;
    }

    ui->label_hour->setText(QString::number((ulong) (Second /3600)));
    ui->label_min->setText(QString::number(Second /60 %60));
}

void SetDelayChargingWin::on_pushButton_dec_clicked()
{
    if(Second >=dSecond)
    {
        Second -=dSecond;
    }
    else Second =0;

    ui->label_hour->setText(QString::number((ulong) (Second /3600)));
    ui->label_min->setText(QString::number(Second /60 %60));
}

void SetDelayChargingWin::on_pushButton_ok_clicked()
{
    m_DelayTimer =Second;
    pCurWin =new DelayChargingWin(this->parentWidget());
    pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
    pCurWin->show();

    this->close();
}

void SetDelayChargingWin::on_pushButton_exit_clicked()
{
    m_ChargerA.UnLock();
    this->close();
    pCurWin =NULL;
}

void SetDelayChargingWin::on_TimerEvent()
{
    if(!(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected()))
    {
        if((m_ChargerA.Command &_C_CHARGEMASK) !=_C_CHARGECOMMAND)
            on_pushButton_exit_clicked();
    }
}

void SetDelayChargingWin::keyPressEvent(QKeyEvent *event)
{
    if(event->key() ==Qt::Key_Right)
    {
        if(ui->pushButton_add->isChecked())
        {
            ui->pushButton_add->setChecked(false);
            ui->pushButton_dec->setChecked(true);
        }
        else if(ui->pushButton_dec->isChecked())
        {
            ui->pushButton_dec->setChecked(false);
            ui->pushButton_ok->setChecked(true);
        }
        else if(ui->pushButton_ok->isChecked())
        {
            ui->pushButton_ok->setChecked(false);
            ui->pushButton_exit->setChecked(true);
        }
        else if(ui->pushButton_exit->isChecked())
        {
            ui->pushButton_exit->setChecked(false);
            ui->pushButton_add->setChecked(true);
        }
    }
    else if(event->key() ==Qt::Key_Return)
    {
        if(ui->pushButton_add->isChecked())
            on_pushButton_add_clicked();
        else if(ui->pushButton_dec->isChecked())
            on_pushButton_dec_clicked();
        else if(ui->pushButton_exit->isChecked())
            on_pushButton_exit_clicked();
        else if(ui->pushButton_ok->isChecked())
            on_pushButton_ok_clicked();
    }
}
