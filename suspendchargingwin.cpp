#include "suspendchargingwin.h"
#include "ui_suspendchargingwin.h"
#include <QThread>
#include "api_function.h"
#include "gvar.h"
#include "hostplatform.h"
#include "messagewindow.h"

SuspendChargingWindow::SuspendChargingWindow(QWidget * parent):
QMainWindow(parent),
ui(new Ui::SuspendChargingWindow),
connIcon(m_OcppClient){
    qWarning() << "SUSPENDWIN: init";
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);

    // 设置倒计时半小时，即30分钟
    int countDown = 30 * 60; // 30分钟 = 30 * 60秒
    countDownTimer = new QTimer(this);
    connect(countDownTimer, &QTimer::timeout, this, &SuspendChargingWindow::updateTime);
    countDownTimer->start(1000); // 每隔1秒触发一次timeout信号
    m_countDown = countDown;

    QString windowStyleSheet = QString("QWidget#centralwidget {background-image: url(%1suspensionWin.%2.jpg);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir)
            .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "EN" : "ZH");

    this->setStyleSheet(windowStyleSheet);

    QString exitButtonStyleSheet = QString("QPushButton{background:transparent; border-image:none;color:white; font-family: Piboto, Calibri, sans-serif; font-size: 24px} "
                                           "QPushButton:checked{border-image: url(%1/PNG/Rect2.png);color:white; font-family: Piboto, Calibri, sans-serif; font-size: 24px}")
            .arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->pushButton_exit->setStyleSheet(exitButtonStyleSheet);
    ui->pushButton_exit->setVisible(true);
    if(charger::getCurrDisplayLang() == charger::DisplayLang::English){
        ui->pushButton_exit->setText(tr("Exit"));
    }else if(charger::getCurrDisplayLang() == charger::DisplayLang::Chinese){
        ui->pushButton_exit->setText(tr("離開"));
    }

    QString pauseButtonStyleSheet = QString("QPushButton{background:transparent; border-image:none;color:white; font-family: Piboto, Calibri, sans-serif; font-size: 24px} "
                                            "QPushButton:checked{border-image: url(%1/PNG/Rect2.png);color:white; font-family: Piboto, Calibri, sans-serif; font-size: 24px}")
            .arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->pushButton_resume->setStyleSheet(pauseButtonStyleSheet);
    ui->pushButton_resume->setVisible(true);
    if(charger::getCurrDisplayLang() == charger::DisplayLang::English){
        ui->pushButton_resume->setText(tr("Resume"));
    }else if(charger::getCurrDisplayLang() == charger::DisplayLang::Chinese){
        ui->pushButton_resume->setText(tr("恢復"));
    }

    cableConnTimer = new QTimer(this);
    connect(cableConnTimer, &QTimer::timeout, this, &SuspendChargingWindow::on_cableConn_timeout);

    ui->pushButton_resume->setChecked(true);

    ui->label_notification->setText(tr(""));

    ++instanceCount;
}

SuspendChargingWindow::~SuspendChargingWindow(){

    delete ui;

    if(countDownTimer){
        delete countDownTimer;
    }
    if(cableConnTimer){
        delete cableConnTimer;
    }
    --instanceCount;
}

void SuspendChargingWindow::updateStyleStrs(charger::DisplayLang lang)
{
    using namespace charger;

    QString langInFn;
    switch (lang) {
    case DisplayLang::Chinese:
    case DisplayLang::SimplifiedChinese:
        langInFn = "ZH"; break;
    case DisplayLang::English: langInFn = "EN"; break;
    }
}

void SuspendChargingWindow::on_pushButton_resume_clicked()
{
    //countDownTimer->stop();

    if(! (m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected())){
        qDebug() <<"resume from suspension, but cable not plugged in. wait for cable reconnected";

        cableConnTimer->start(250);
    }else{
        this->close();
        finishSuspension(1);
    }
}

void SuspendChargingWindow::on_cableConn_timeout()
{
    QString style = "color:white; font-family: Piboto, Calibri, sans-serif; font-size: 22px";
    if(!m_ChargerA.IsCablePlugIn()){
        ui->label_notification->setStyleSheet(style);
        if(charger::getCurrDisplayLang() == charger::DisplayLang::English){
            ui->label_notification->setText(tr("Please Plug in to Connect the Cable"));
        }else if(charger::getCurrDisplayLang() == charger::DisplayLang::Chinese){
            ui->label_notification->setText(tr("請插槍連接電纜"));
        }
    }else{
        if(m_ChargerA.IsEVConnected())
        {
            countDownTimer->stop();
            cableConnTimer->stop();
            this->close();
            finishSuspension(1);
        }else{
            qDebug() <<"resume from suspension, cable plugged in but the EV side. wait for cable reconnected";
            ui->label_notification->setStyleSheet(style);
            if(charger::getCurrDisplayLang() == charger::DisplayLang::English){
                ui->label_notification->setText(tr("Please Connect the Cable To EV"));
            }else if(charger::getCurrDisplayLang() == charger::DisplayLang::Chinese){
                ui->label_notification->setText(tr("請將電纜連接到電車"));
            }
        }
    }
}

void SuspendChargingWindow::updateTime()
{
    if (m_countDown > 0) {
        m_countDown--;
        int i_min = m_countDown/60;
        int i_sec = m_countDown%60;
        QString str_min = QString("%1").arg(i_min, 2, 10, QLatin1Char('0'));
        QString str_sec = QString("%1").arg(i_sec, 2, 10, QLatin1Char('0'));

        ui->label_min->setText(str_min) ;
        ui->label_sec->setText(str_sec) ;
    } else {
        on_pushButton_exit_clicked();
    }
}

void SuspendChargingWindow::on_pushButton_exit_clicked()
{
    countDownTimer->stop();
    countDownTimer->deleteLater();
    cableConnTimer->stop();
    cableConnTimer->deleteLater();

    m_ChargerA.UnLock();
    m_ChargerA.ChargerIdle();
    this->close();
    finishSuspension(0);
}

void SuspendChargingWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() ==Qt::Key_Right)
    {
        ui->pushButton_resume->setCheckable(true);
        ui->pushButton_exit->setCheckable(true);
        if(ui->pushButton_resume->isChecked())
        {
            ui->pushButton_resume->setChecked(false);
            ui->pushButton_exit->setChecked(true);
        }
        else //if(ui->pushButton_exit->isChecked())
        {
            ui->pushButton_resume->setChecked(true);
            ui->pushButton_exit->setChecked(false);
        }
        //qWarning()<<"Set Checked,resume"<<event->key()<<ui->pushButton_resume->isChecked()<<",,,exit"<<ui->pushButton_exit->isChecked();
    }
    else if(event->key() ==Qt::Key_Return)
    {
        if(ui->pushButton_resume->isChecked())
            on_pushButton_resume_clicked();
        else if(ui->pushButton_exit->isChecked())
            on_pushButton_exit_clicked();
    }
}

int SuspendChargingWindow::instanceCount = 0;
