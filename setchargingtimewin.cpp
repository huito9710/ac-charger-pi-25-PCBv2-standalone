#include "setchargingtimewin.h"
#include "ui_setchargingtimewin.h"
#include "waittingwindow.h"
#include "api_function.h"
#include "gvar.h"
#include "hostplatform.h"
#include "chargerconfig.h"

SetChargingTimeWin::SetChargingTimeWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SetChargingTimeWin),
    connIcon(m_OcppClient)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    updateStyleStrs(charger::getCurrDisplayLang());
    this->setStyleSheet(winBgStyle);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);

    Second =0;
    m_ChargerA.ChargeTimeValue =0;

    QString itemvalue="30";     //30'
    GetSetting(QStringLiteral("[INCCHARGINGTIME]"), itemvalue);
    timeSelectIndex = 0;
    if(timeSelection[0] != 0)
    {
        Second = timeSelection[timeSelectIndex];
        m_ChargerA.ChargeTimeValue = Second;
        m_ChargerA.RemainTime = Second;
        ui->label_hour->setText(QString::number((ulong) (Second /3600)));
        ui->label_min->setText(QString::number(Second /60 %60));
    }
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

SetChargingTimeWin::~SetChargingTimeWin()
{
    delete ui;
}

void SetChargingTimeWin::updateStyleStrs(charger::DisplayLang lang)
{
    using namespace charger;

    QString langInFn;
    switch (lang) {
    case DisplayLang::Chinese:
    case DisplayLang::SimplifiedChinese:
        langInFn = "ZH"; break;
    case DisplayLang::English: langInFn = "EN"; break;
    }
    winBgStyle = QString("QWidget#centralwidget {background-image: url(%1setChargingTimeWin.%2.jpg);}").arg(Platform::instance().guiImageFileBaseDir).arg(langInFn);
}

void SetChargingTimeWin::on_pushButton_add_clicked()
{
    QString maxtime ="6000";    //MAX 100h

    GetSetting(QStringLiteral("[MAXCHARGINGTIME]"), maxtime);
    Second +=dSecond;
    if(Second <=maxtime.toULong()*60)
    {
        m_ChargerA.ChargeTimeValue =Second;
        ui->label_hour->setText(QString::number((ulong) (Second /3600)));
        ui->label_min->setText(QString::number(Second /60 %60));
    }
    else Second =m_ChargerA.ChargeTimeValue;

    if(timeSelection[0] != 0)
    {
        timeSelectIndex += 1;
        if(timeSelectIndex == 4)timeSelectIndex = 3;
        Second = timeSelection[timeSelectIndex];
        m_ChargerA.ChargeTimeValue = Second;
        ui->label_hour->setText(QString::number((ulong) (Second /3600)));
        ui->label_min->setText(QString::number(Second /60 %60));
    }
}

void SetChargingTimeWin::on_pushButton_dec_clicked()
{
    if(Second >=dSecond)
    {
        Second -=dSecond;
        m_ChargerA.ChargeTimeValue =Second;
        ui->label_hour->setText(QString::number((ulong) (Second /3600)));
        ui->label_min->setText(QString::number(Second /60 %60));
    }
    
    if(timeSelection[0] != 0)
    {
        timeSelectIndex -= 1;
        if(timeSelectIndex < 0)timeSelectIndex = 0;
        Second = timeSelection[timeSelectIndex];
        m_ChargerA.ChargeTimeValue = Second;
        ui->label_hour->setText(QString::number((ulong) (Second /3600)));
        ui->label_min->setText(QString::number(Second /60 %60));
    }
}

void SetChargingTimeWin::on_pushButton_ok_clicked()
{
    if(m_ChargerA.ChargeTimeValue >0)
    {
        if (charger::overrideChargeTimeValue())
            m_ChargerA.ChargeTimeValue = charger::getOverrideChargeTimeValue();

        m_ChargerA.RemainTime =m_ChargerA.ChargeTimeValue;
        chargeTimeSet(true);
    }
}

void SetChargingTimeWin::on_pushButton_exit_clicked()
{
    m_ChargerA.UnLock();
    m_ChargerA.ChargerIdle();
    chargeTimeSet(false);
}

void SetChargingTimeWin::keyPressEvent(QKeyEvent *event)
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

