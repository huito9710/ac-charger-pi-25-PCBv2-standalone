#include "delaychargingwin.h"
#include "ui_delaychargingwin.h"
#include "waittingwindow.h"
#include "gvar.h"
#include "hostplatform.h"

DelayChargingWin::DelayChargingWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DelayChargingWin),
    winBgStyle(QString("QWidget#centralwidget {background-image: url(%1delayTimerWin.%2.jpg);}")
               .arg(charger::Platform::instance().guiImageFileBaseDir)
               .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English?"EN":"ZH"))
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    m_Timer =new QTimer();
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
    m_Timer->start(1000);

    this->setStyleSheet(winBgStyle);

    ui->label_hour->setText(QString::number((ulong) (m_DelayTimer /3600)));
    ui->label_min->setText(QString::number(m_DelayTimer /60 %60));

    if(charger::isHardKeySupportRequired())
    {
        ui->pushButton_exit->setCheckable(true);
        ui->pushButton_exit->setChecked(true);
    }
}

DelayChargingWin::~DelayChargingWin()
{
    delete ui;

    if(m_Timer)
    {
        m_Timer->stop();
        delete m_Timer;
    }
}

void DelayChargingWin::on_pushButton_exit_clicked()
{
    m_ChargerA.UnLock();
    m_ChargerA.ChargerIdle();
    this->close();
    pCurWin =NULL;
}

void DelayChargingWin::on_TimerEvent()
{
    if((!(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected()))
            && (m_ChargerA.ConnType() == CCharger::ConnectorType::EvConnector
                || m_ChargerA.ConnType() == CCharger::ConnectorType::AC63A_NoLock))
    {
        if((m_ChargerA.Command &_C_CHARGEMASK) !=_C_CHARGECOMMAND)
            on_pushButton_exit_clicked();
    }
    else
    {
        if(m_DelayTimer) m_DelayTimer--;
        ui->label_hour->setText(QString::number((ulong) ((m_DelayTimer +59)/3600)));
        ui->label_min->setText(QString::number((m_DelayTimer+59) /60 %60));

        if(!m_DelayTimer)       //计时完, 自动充满
        {
            m_ChargerA.StartChargeToFull();

            pCurWin =new WaittingWindow(this->parentWidget());
            pCurWin->setWindowState(Qt::WindowFullScreen);
            pCurWin->show();
            this->close();
        }
    }
}

void DelayChargingWin::keyPressEvent(QKeyEvent *event)
{
    if(event->key() ==Qt::Key_Return)
    {
        on_pushButton_exit_clicked();
    }
}
