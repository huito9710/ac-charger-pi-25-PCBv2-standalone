#include "cablewin.h"
#include "ui_cablewin.h"
#include "messagewindow.h"
#include "paymentmethodwindow.h"
#include "chargerequestpassword.h"
#include "rfidwindow.h"
#include "waittingwindow.h"
#include <QtGui>
#include "ccharger.h"
#include "videoplay.h"
#include "gvar.h"
#include "api_function.h"
#include "./OCPP/ocppclient.h"      //仅用于取IP
#include "hostplatform.h"
#include "passwordupdater.h"
#include "chargingmodeimmediatewin.h"
#include "statusledctrl.h"

CableWin::CableWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CableWin),
    connIcon(m_OcppClient),
    evConnLabelStyle(QString("background:transparent; image: url(%1right.png);").arg(charger::Platform::instance().guiImageFileBaseDir)),
    evDisconnLabelStyle(QString("background:transparent; image: url(%1wrong.png);").arg(charger::Platform::instance().guiImageFileBaseDir))
{
    qWarning() << "CABLEWIN: init";
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);

    connect(&connMonitorTimer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
    connMonitorTimer.start(200);    //500=0.5" 影响空闲10'返回主界面
    lockDelay =0;
    idleTimeout =0;
    m_ChargerA.SetConnType(charger::getAC63ANoLockSupported()?
                               CCharger::ConnectorType::AC63A_NoLock
                             : CCharger::ConnectorType::EvConnector);   //从mainwindow定时事件中移到此处

    QString windowStyleSheet = QString("QWidget#centralwidget {background-image: url(%1cableWin.%2.jpg);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir)
            .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "EN" : "ZH");

    this->setStyleSheet(windowStyleSheet);

    ui->label_BK->setStyleSheet(tutorial1labelStyle);
    ui->label_BK->setVisible(VideoPlay::tutorialsFilePresent());

    ui->label_msg->hide();

    if (charger::isStandaloneMode()) {
        ui->ocppStateLabel->setVisible(false);
    }
    else {
        QString style = "color:red; font-family: Piboto, Calibri, sans-serif; font-size: 22px";
        ui->ocppStateLabel->setStyleSheet(style);
        ui->ocppStateLabel->clear();
        ui->ocppStateLabel->setVisible(true);
    }

    // Overrides the unix-paths defined in pushbuttons
    QString pushButtonStyleSheet = QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1/PNG/Rect2.png);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->pushButton->setStyleSheet(pushButtonStyleSheet);
    ui->langChangeButton->setStyleSheet(pushButtonStyleSheet);
    QString videoButtonStyleSheet = QString("QPushButton{background:transparent; image:none;} QPushButton:checked{border-image: url(%1/PNG/Rect5.png);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->tutorialButton->setStyleSheet(videoButtonStyleSheet);
    bool showTutorialButton = VideoPlay::tutorialsFilePresent();
    ui->tutorialButton->setVisible(showTutorialButton);
    if (showTutorialButton)
        ui->connStatusIcon->move(ui->tutorialButton->x() + ui->tutorialButton->width(), ui->connStatusIcon->y() + 4);
    QString hiddenButtonStyleSheet = QString("background:transparent; border-image:none;");
    ui->pushButton_4->setStyleSheet(hiddenButtonStyleSheet);
    // settings button (password change)
    QString passwordChangeButtonStyleSheet = QString("QPushButton{background:transparent; border-image:url(%1PNG/settingsSmallButton.png); image:none;} QPushButton:checked{image: url(%1PNG/Rect2.png);}").arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->passwordChangeButton->setStyleSheet(passwordChangeButtonStyleSheet);
    // only make it visible in standalone-mode
    ui->passwordChangeButton->setVisible(charger::passwordAuthEnabled());

    if(charger::isHardKeySupportRequired())
    {
        ui->pushButton->setCheckable(true);
        ui->langChangeButton->setCheckable(true);
        ui->tutorialButton->setCheckable(VideoPlay::tutorialsFilePresent());
        ui->pushButton_4->setCheckable(true);
        ui->passwordChangeButton->setCheckable(charger::passwordAuthEnabled());

        ui->pushButton->setChecked(true);
    }

    ui->label_version->setText(charger::getSoftwareVersion());
    ui->label_version->setVisible(true);
}

CableWin::~CableWin()
{
    delete ui;
    if(connMonitorTimer.isActive())
        connMonitorTimer.stop();
}

void CableWin::initStyleStrings(charger::DisplayLang lang)
{
    using namespace charger;
    QString langFnSymbol;
    switch (lang)
    {
    case DisplayLang::English:
        langFnSymbol = "EN";
        break;
    case DisplayLang::Chinese:
        langFnSymbol = "ZH";
        break;
    default:
        Q_ASSERT(false);
    }


    cableWindowStyle = QString("QWidget#centralwidget {background-image: url(%1cableWin.%2.jpg);}").arg(Platform::instance().guiImageFileBaseDir).arg(langFnSymbol);
    socketTypeWindowStyle = QString("QWidget#centralwidget {background-image: url(%1socketTypeWin.%2.jpg);}").arg(Platform::instance().guiImageFileBaseDir).arg(langFnSymbol);
    tutorial0labelStyle = QString("background:transparent; border-image: url(%1Tutorials0.%2.png);").arg(Platform::instance().guiImageFileBaseDir).arg(langFnSymbol);
    tutorial1labelStyle = QString("background:transparent; border-image: url(%1Tutorials1.%2.png);").arg(Platform::instance().guiImageFileBaseDir).arg(langFnSymbol);
}

void CableWin::on_tutorialButton_clicked()
{ //BACK: 播教学视频
    qWarning() << "CABLEWIN: Tutorial button clicked";
    if(pVideoWin ==NULL)       //*不能准确测文件
    {
        m_VideoNo =1;
        pVideoWin =new VideoPlay(this);
        pVideoWin->setWindowState(Qt::WindowFullScreen);
        pVideoWin->show();
    }
    //m_ChargerA.SaveChargeRecord();
}

void CableWin::on_pushButton_clicked()
{ //HOME:
    qWarning() << "CABLEWIN: Closing this window";
    m_ChargerA.UnLock();
    this->close();
    pCurWin =NULL;
}

void CableWin::on_pushButton_4_clicked()
{ //NEXT:
    //seems this button is abandoned.
    qWarning() << "CABLEWIN: Push button 4 clicked";
    if(m_ChargerA.IsCablePlugIn() &&m_ChargerA.IsUnLock())
    {
        m_ChargerA.Lock();
        //QThread::sleep(2);
    }

    if(m_ChargerA.IsCablePlugIn() &&!m_ChargerA.IsUnLock())
    {
        if(m_ChargerA.IsEVConnected())
        {
            if(charger::GetSkipPayment())
            {
                qWarning() << "CABLEWIN: Skip payment, go to charge";
                m_ChargerA.IdTag = "SKIP";
                switch (charger::getChargingMode()) {
                case charger::ChargeMode::UserSelect:
                    qWarning() << "CABLEWIN: Go to user select page";
                    pCurWin = new ChargingModeImmediateWin(this->parentWidget());
                    pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
                    pCurWin->show();
                    this->close();
                    break;
                case charger::ChargeMode::ChargeToFull:
                    qWarning() << "CABLEWIN: Start charge to full";
                    // Start Charging Now
                    m_ChargerA.StartChargeToFull();
                    pCurWin =new WaittingWindow(this->parentWidget());
                    pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
                    pCurWin->show();
                    this->close();
                    qDebug() <<"Auto in Mode";
                    break;
                default:
                    qWarning() << "CABLEWIN: Unexpected charging-mode value " << static_cast<int>(charger::getChargingMode());
                    break;
                }
            }
            else
            {
                pCurWin =new PaymentMethodWindow(this->parentWidget());
                pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
                pCurWin->show();

                this->close();
            }

        }
        else
        {
            m_ChargerA.VendorErrorCode ="";
            pMsgWin =new MessageWindow(this);
            pMsgWin->setWindowState(charger::Platform::instance().defaultWinState);
            pMsgWin->setStyleSheet(QString("background-image: url(/home/pi/Desktop/BK/BG-P.3.car.%1.jpg);")
                                   .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "en" : "cn"));
            pMsgWin->show();
        }
    }
    else
    {
        pMsgWin =new MessageWindow(this);
        pMsgWin->setWindowState(charger::Platform::instance().defaultWinState);

        pMsgWin->setStyleSheet(QString("background-image: url(/home/pi/Desktop/BK/BG-P.3.cp.%1.jpg);")
                               .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "en" : "cn"));
        pMsgWin->show();
    }
}

void CableWin::changeEvent(QEvent *event)
{
    qWarning() << "CABLEWIN: Change event";
    if (event->type() == QEvent::LanguageChange) {
        initStyleStrings(charger::getCurrDisplayLang());
        this->setStyleSheet(cableWindowStyle);
        ui->label_BK->setStyleSheet(tutorial1labelStyle);
        UpdateMessage();
    }
}

void CableWin::on_langChangeButton_clicked()
{
    qWarning() << "CABLEWIN: Language change button clicked";
    charger::toggleDisplayLang();
    //测试开启充电:
//    m_ChargerA.ChargeTimeValue =300;
//    m_ChargerA.RemainTime =m_ChargerA.ChargeTimeValue;
//    m_ChargerA.StartCharge();
}

void CableWin::on_TimerEvent()
{
    //qWarning() << "CABLEWIN: Timer event";
    if(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected() && !pMsgWin)
    {
        qWarning() << "CABLEWIN: Timer event. Cable plugged and EV connected";
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::yellow);
        if(charger::reduceScreenTransitions() || lockDelay++ >5)
        {
            using namespace charger;
            EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
            Q_ASSERT(!reg.hasActiveTransaction());
            switch(getChargingMode()) {
            case ChargeMode::NoAuthChargeToFull: //插电直充模式 自动充满 (应关闭网络版)
                qWarning() << "CABLEWIN: Timer event. Cable plugged and EV connected. No auth charge to full";
                m_ChargerA.ChargeTimeValue =100*60*60;  //100h
                m_ChargerA.RemainTime =m_ChargerA.ChargeTimeValue;
                m_ChargerA.StartCharge();

                pCurWin =new WaittingWindow(this->parentWidget());
                pCurWin->setWindowState(Qt::WindowFullScreen);
                pCurWin->show();
                this->close();
                qDebug()<<"插电直充";
                break;
            default:
                if(charger::isStandaloneMode())
                { //bNetEnable条件多余的*
                    qWarning() << "CABLEWIN: Timer event. Cable plugged and EV connected. Standalone mode";
                    if(charger::getRfidSupported()) {
                        qWarning() << "CABLEWIN: Timer event. Cable plugged and EV connected. Standalone mode RFID. Start RFID page";
                        pCurWin =new RFIDWindow(this->parentWidget());
                    }
                    else if (charger::isStandaloneMode()){
                        qWarning() << "CABLEWIN: Timer event. Cable plugged and EV connected. Standalone mode password. Start password page";
                        // Password is only available to standalone-mode, not when OCPP went offline
                        ChargeRequestPassword *reqPassword = new ChargeRequestPassword();
                        // reqPassword will de-allocate itself when the there is no more work to be done
                        // (lack of reference to reqPassword object)
                        pCurWin = reqPassword->Start(this->parentWidget());
                    }
                }
                else if (!m_OcppClient.isOffline() || charger::allowOfflineCharging())
                {
                    if(charger::GetSkipPayment())
                    {
                        qWarning() << "CABLEWIN: Timer event. Skip payment, go to charge";
                        m_ChargerA.IdTag = "SKIP";
                        switch (charger::getChargingMode()) {
                        case charger::ChargeMode::UserSelect:
                            qWarning() << "CABLEWIN: Go to user select page";
                            pCurWin = new ChargingModeImmediateWin(this->parentWidget());
                            pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
                            pCurWin->show();
                            this->close();
                            break;
                        case charger::ChargeMode::ChargeToFull:
                            qWarning() << "CABLEWIN: Timer event. Start charge to full";
                            // Start Charging Now
                            m_ChargerA.StartChargeToFull();
                            pCurWin =new WaittingWindow(this->parentWidget());
                            pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
                            pCurWin->show();
                            this->close();
                            qDebug() <<"Auto in Mode";
                            break;
                        default:
                            qWarning() << "CABLEWIN: Timer event. Unexpected charging-mode value " << static_cast<int>(charger::getChargingMode());
                            break;
                        }
                    }
                    else
                    {
                        qWarning() << "CABLEWIN: Timer event. Cable plugged and EV connected. Online mode. Start payment method page";
                        pCurWin =new PaymentMethodWindow(this->parentWidget());
                    }
                }
                if (pCurWin != this) {
                    pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
                    pCurWin->show();
                    this->close();
                }
                break;
            }
            //return;   //应该不用执行下面的语句
        }
    }
    else
    {
        lockDelay =0;
        if(StatusLedCtrl::instance().getState() == StatusLedCtrl::State::yellow)
        {
//            if(charger::GetQueueingMode())
//            {
//                qWarning() << "queueing mode,unplug from preparing, change LED light to blue";
//                StatusLedCtrl::instance().setState(StatusLedCtrl::State::blue);
//            }
//            else
//            {
                qWarning() << "not queueing mode,unplug from preparing, change LED light to green";
                StatusLedCtrl::instance().setState(StatusLedCtrl::State::green);
//            }
        }

    }

    UpdateMessage();

    //无电缆超时返回待机界面:
    if(m_ChargerA.IsCablePlugIn()
            || (m_ChargerA.ConnType() == CCharger::ConnectorType::EvConnector)
            || (m_ChargerA.ConnType() == CCharger::ConnectorType::AC63A_NoLock))
        idleTimeout =0;
    else if(++idleTimeout >60*10*5)  //60*10* 2 =1200
    {
        idleTimeout =30000;
        if((!pMsgWin) && !pVideoWin)
            on_pushButton_clicked();
    }

    initStyleStrings(charger::getCurrDisplayLang());

    if(m_ChargerA.IsCablePlugIn() || m_ChargerA.IsEVConnected())
    {
        ui->tutorialButton->setEnabled(false);
        ui->label_BK->setStyleSheet(tutorial0labelStyle);
    }
    else
    {
        ui->tutorialButton->setEnabled(true);
        ui->label_BK->setStyleSheet(tutorial1labelStyle);
    }

    ui->label_state->setText(QString("%1-%2").arg(m_ChargerA.State, 8, 16, QLatin1Char('0')).arg((int)m_ChargerA.Command, 0, 16).toUpper());
    ui->label_state->setVisible(false); // debug-purpose
    //ui->label_state->setText(QString("%1-%2\t%3").arg(m_ChargerA.State, 8, 16, QLatin1Char('0')).arg((int)m_ChargerA.Command, 0, 16).toUpper().arg(m_OcppClient.GetLocalIP()));

    if (ui->ocppStateLabel->isVisible()) {
        QString labelText = m_OcppClient.isOffline()? tr("Safe-Mode") : "";
        ui->ocppStateLabel->setText(labelText);
    }
}

void CableWin::UpdateMessage()
{
    //qWarning() << "CABLEWIN: Update message";
    if(m_ChargerA.IsCablePlugIn())
    {
        ui->label_charger->setStyleSheet(evConnLabelStyle);
        if(m_ChargerA.IsEVConnected())
        {
            ui->label_ev->setStyleSheet(evConnLabelStyle);
            ui->label_msg->setText(tr("Cable Maximum Current") + QString("%1A").arg(m_ChargerA.GetCableMaxCurrent()));
        }
        else
        {
            ui->label_ev->setStyleSheet(evDisconnLabelStyle);
            ui->label_msg->setText(tr("Please Connect Cable"));
        }
    }
    else
    {
        ui->label_charger->setStyleSheet(evDisconnLabelStyle);
        ui->label_msg->setText(tr("Please Connect Cable"));

        if(m_ChargerA.IsEVConnected())
        {
            ui->label_ev->setStyleSheet(evConnLabelStyle);
        }
        else
        {
            ui->label_ev->setStyleSheet(evDisconnLabelStyle);
        }
    }
}

void CableWin::keyPressEvent(QKeyEvent *event)
{
    qWarning() << "CABLEWIN: Key press event";
    switch(event->key()) {
    case Qt::Key_Right:
    case Qt::Key_PageDown:
        // PB2 -> PB3 -> PB1 -> PasswordChange
        if(ui->langChangeButton->isChecked())
        {
            ui->langChangeButton->setChecked(false);
            if(ui->tutorialButton->isEnabled() &&ui->tutorialButton->isVisible())
                ui->tutorialButton->setChecked(true);
            else ui->pushButton->setChecked(true);
        }
        else if(ui->tutorialButton->isChecked())
        {
            ui->tutorialButton->setChecked(false);
            ui->pushButton->setChecked(true);
        }
        else if(ui->pushButton->isChecked())
        {
            ui->pushButton->setChecked(false);
            if (ui->passwordChangeButton->isVisible())
                ui->passwordChangeButton->setChecked(true);
            else
                ui->langChangeButton->setChecked(true);
        }
        else if(ui->passwordChangeButton->isChecked())
        {
            ui->passwordChangeButton->setChecked(false);
            ui->langChangeButton->setChecked(true);
        }
        break;
    case Qt::Key_Return:
        //qDebug()<<event->text()<<"="<<event->isAutoRepeat()<<"--"<<event->count();
        if(ui->langChangeButton->isChecked())
            on_langChangeButton_clicked();
        else if(ui->tutorialButton->isChecked())
            on_tutorialButton_clicked();
        else if(ui->pushButton->isChecked())
            on_pushButton_clicked();
        else if(ui->passwordChangeButton->isChecked())
            on_passwordChangeButton_clicked();
        break;
    default:
        qWarning() << "Unhandled key event (" << event->key() << ").";
        break;
    }
}

void CableWin::on_passwordChangeButton_clicked()
{
    qWarning() << "CABLEWIN: Password change button clicked";
    PasswordUpdater *pwUpdater = new PasswordUpdater(nullptr);
    pwUpdater->start(); // object will free itself when done.
}

