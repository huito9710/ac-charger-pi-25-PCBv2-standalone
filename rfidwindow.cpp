#include "rfidwindow.h"
#include "ui_rfidwindow.h"
#include "chargingmodewin.h"
#include "waittingwindow.h"
#include "api_function.h"
#include "gvar.h"
#include "hostplatform.h"
#include "chargerconfig.h"
#include "statusledctrl.h"
#include <stdexcept>


RFIDWindow::RFIDWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RFIDWindow),
    connIcon(m_OcppClient),
    noReaderImageFilePath(QString("%1RFID_NoReader.png").arg(charger::Platform::instance().guiImageFileBaseDir)),
    hwStatusLabelStyleStr(QString("background:transparent; image: url(%1);").arg(noReaderImageFilePath)),
    winBgStyleStr(QString("QWidget#centralwidget {background-image: url(%1RFID_ReadCard.jpg);}")
                  .arg(charger::Platform::instance().guiImageFileBaseDir))
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);

    this->setStyleSheet(winBgStyleStr);

    QString quitButtonStyleSheet = QString("QPushButton{background:transparent; border-image:url(%1PNG/quit.%2.png); image:none;} QPushButton:checked{image: url(%1PNG/Rect2.png);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir)
            .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "EN" : "ZH");
    ui->pushButton_exit->setStyleSheet(quitButtonStyleSheet);
    if (charger::isHardKeySupportRequired()) {
        ui->pushButton_exit->setCheckable(true);
        ui->pushButton_exit->setChecked(true);
    }
    m_Timer =new QTimer();
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
    connect(&RFIDThread, SIGNAL(RFIDCard(QByteArray)), this, SLOT(on_RFIDCard(QByteArray)));
    m_Timer->start(500);

    setWinBg(RfidReqStatus::PresentCard, UserRequest::Authorize);

    //RFIDThread.StartSlave(charger::Platform::instance().rfidSerialPortName, 500);
    RFIDThread.StartSlave(500);
    RFIDThread.ReadCard();

    m_Delay =0;

    QString itemvalue="";
    ui->secondaryResultLabel->setVisible(false);
    GetSetting(QStringLiteral("[SHOW CARD_ID]"), itemvalue);
    if(itemvalue =="1") ui->secondaryResultLabel->setVisible(true);

    ui->label_HW_status->setVisible(false);
    if(QFile::exists(noReaderImageFilePath))
        ui->label_HW_status->setStyleSheet(hwStatusLabelStyleStr);
}

RFIDWindow::~RFIDWindow()
{
    delete ui;

    if(m_Timer)
    {
        m_Timer->stop();
        delete m_Timer;
    }
}

void RFIDWindow::setWinBg(RfidReqStatus rfidReqSt, UserRequest usrReq)
{
    updateWinBgStyle(rfidReqSt, usrReq, charger::getCurrDisplayLang());
    this->setStyleSheet(winBgStyleStr);
}

void RFIDWindow::on_pushButton_exit_clicked()
{
    if(pMsgWin ==NULL)
        StatusLedCtrl::instance().clear();    //关闭PCB红灯
    if(pCurWin ==this)  //开启充电时才会有设置pCurWin=本窗口
    {
        pCurWin =NULL;
        this->close();
    }
    else this->hide();
}

void RFIDWindow::on_TimerEvent()
{
    if(m_StayTimer) m_StayTimer--;

    if(!this->isHidden())
    {
        if(RFIDThread.GetStatus())
            ui->label_HW_status->setVisible(true);
        else
            ui->label_HW_status->setVisible(false);
    }

    if((m_ChargerA.ConnType() == CCharger::ConnectorType::EvConnector
        || m_ChargerA.ConnType() == CCharger::ConnectorType::AC63A_NoLock)
        && (!(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected())))
    {
        if((m_ChargerA.Command &_C_CHARGEMASK) !=_C_CHARGECOMMAND)
            on_pushButton_exit_clicked();
    }
    else
    {
        if(m_Delay)
        {
            if (charger::reduceScreenTransitions())
                m_Delay = 0; // do-not-wait
            else
                m_Delay--;
            if(!m_Delay)
            {
                if(pMsgWin ==NULL)
                    StatusLedCtrl::instance().clear();
                if(pCurWin ==this)
                {
                    ui->secondaryResultLabel->setText("");     //test
                    CardNo.clear();
                    RFIDThread.ReadCardAgain();
                    setWinBg(RfidReqStatus::PresentCard, UserRequest::Authorize);
                }
                else
                {
                    if(this->windowTitle() =="STOP ERR")
                    {
                        CardNo.clear();
                        RFIDThread.ReadCardAgain();
                        this->hide();
                        setWinBg(RfidReqStatus::PresentCard, UserRequest::Authorize);
                    }
                    else
                    {
                        m_ChargerA.StopCharge();
                        m_ChargerA.UnLock();
                        //this->close(); //停充后会在充电窗口按钮出错
                    }
                }
            }
        }
    }
}

void RFIDWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() ==Qt::Key_Return)
    {
        on_pushButton_exit_clicked();
    }
}

void RFIDWindow::on_RFIDCard(QByteArray cardno)
{
    if(m_StayTimer) { m_StayTimer =5*2; RFIDThread.ReadCardAgain(); return; } //已读过的卡还未取开

    CardNo.clear();
    CardNo = cardno.toHex().toUpper();

    qDebug() <<CardNo <<"=="<<m_ChargerA.IdTag;
    if((IsValidCard(CardNo)    &&((pCurWin ==this)||(m_ChargerA.IdTag ==CardNo)))
       ||((m_ChargerA.IdTag ==CardNo) && !charger::isStandaloneMode() )  )
    {
        if(pCurWin ==this)
        {
            using namespace charger;

            m_ChargerA.IdTag =CardNo;
            switch (getChargingMode()) {
            case ChargeMode::UserSelect: //普通模式 设置定时
                pCurWin =new ChargingModeWin(this->parentWidget());
                pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
                pCurWin->show();
                this->close();
                break;
            default: //简易模式 自动充满
                m_ChargerA.StartChargeToFull();

                pCurWin =new WaittingWindow(this->parentWidget());
                pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
                pCurWin->show();
                this->close();
                qDebug()<<"Auto after RFID";
                break;
            }
        }
        else
        {
            setWinBg(RfidReqStatus::Success, UserRequest::StopCharge);
            if(m_Delay ==0) m_Delay =2;    //延时1"//4: 2"
            if(this->isHidden()) this->show();
            this->setWindowTitle("STOP");
        }
    }
    else
    {
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashingred);   //5
        if(m_Delay ==0) m_Delay =10;    //延时5"

        if(m_ChargerA.Status !=ocpp::ChargerStatus::Charging)
        {
            qDebug() <<"CardNo:" <<CardNo;  //test
            ui->secondaryResultLabel->setText(CardNo);     //test RFID Card ID
            setWinBg(RfidReqStatus::Failed, UserRequest::Authorize);
        }
        else
        {
            this->setWindowTitle("STOP ERR");
            setWinBg(RfidReqStatus::Failed, UserRequest::StopCharge);
            if(this->isHidden()) this->show();
        }
    }
}

void RFIDWindow::WaitRemoveCard()
{
    m_StayTimer =5*2;
}

void RFIDWindow::updateWinBgStyle(RfidReqStatus rfidReqSt, UserRequest usrReq, charger::DisplayLang lang)
{
    using namespace charger;

    QString langInFn;
    switch(lang){
    case DisplayLang::Chinese: langInFn = "ZH"; break;
    case DisplayLang::English: langInFn = "EN"; break;
    default:
        throw std::invalid_argument("Unxpected DisplayLang value");
    }

    QString reqInFn;
    switch (usrReq) {
    case UserRequest::Authorize: reqInFn = "Start";
        ui->primaryResultLabel->setText("Start Scanning");
        break;
    case UserRequest::StopCharge: reqInFn = "Stop";
        ui->primaryResultLabel->setText("Stop Scanning");
        break;
    default:
        ui->primaryResultLabel->setText("");
        throw std::invalid_argument("Unxpected UserRequest value");
    }

    QString fileName;
    switch (rfidReqSt){
    case RfidReqStatus::PresentCard: fileName = QString("RFID_Start.%1.jpg").arg(langInFn);
        ui->secondaryResultLabel->setText("");
        break;
    case RfidReqStatus::Success:
        fileName = QString("RFID_%1OK.%2.jpg").arg(reqInFn).arg(langInFn);
        ui->secondaryResultLabel->setText("RFID Authentication Succeeded");
        break;
    case RfidReqStatus::Failed:
        fileName = QString("RFID_%1Err.%2.jpg").arg(reqInFn).arg(langInFn);
        ui->secondaryResultLabel->setText("RFID Authentication Failed");
        break;
    }

    winBgStyleStr = QString("background-image: url(%1%2);").arg(Platform::instance().guiImageFileBaseDir).arg(fileName);
}

