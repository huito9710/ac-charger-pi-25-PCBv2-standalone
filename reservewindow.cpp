#include "reservewindow.h"
#include "ui_reservewindow.h"
#include "hostplatform.h"
#include "gvar.h"
#include "api_function.h"
#include "chargingmodewin.h"
#include "waittingwindow.h"
#include "chargingmodeimmediatewin.h"
#include "statusledctrl.h"
#include "cablewin.h"
#include "sockettype.h"

reservewindow::reservewindow(QWidget *parent) :
    QMainWindow(parent),
    rfidAuthStarted(false),
    ui(new Ui::reservewindow)
    //connIcon(m_OcppClient)
{
    try
    {
        this->setAttribute(Qt::WA_DeleteOnClose);
        
        ui->setupUi(this);
        qWarning() << "reservewindow: setup ui";

        msgStyle = QString("qproperty-alignment: 'AlignTop | AlignHCenter'; color:white; font-family: Piboto, Calibri, sans-serif; font-size: 22px");
        ui->msg->setStyleSheet(msgStyle);
        ui->msg->setVisible(true);
        ui->msg->setText("This charger is reserved\n此充電器已預留");

        this->setStyleSheet(QString("QMainWindow  {background-image: url(%1chargingWin.jpg);}").arg(charger::Platform::instance().guiImageFileBaseDir));

        m_Timer =new QTimer();
        connect(m_Timer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
        m_Timer->start(500);

        // connect(&RFIDThread, SIGNAL(RFIDCard(QByteArray)), this, SLOT(on_RFIDCard(QByteArray)));

        // RFIDThread.StartSlave(charger::Platform::instance().rfidSerialPortName, 500);
        // RFIDThread.ReadCard();

        rfidAuthorizer = std::make_unique<ChargerRequestRfid>(this);
        connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestAccepted, this, &reservewindow::onRequestAccepted);
        connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestCanceled, this, &reservewindow::onRequestCanceled);
        QTimer::singleShot(0, this, &reservewindow::startRfidAuthorizor);

        connect (&m_ChargerA, &CCharger::chargeCommandAccepted, this, &reservewindow::onChargeCommandAccepted);

        m_Delay =0;
        msg_Delay = 0;

        StatusLedCtrl::instance().setState(StatusLedCtrl::State::purple);
    }
    catch(const std::exception& e)
    {
        qWarning() << e.what();
    }
    

}

reservewindow::~reservewindow()
{
    qWarning() << "reservewindow: ~reservewindow";
    delete ui;

    if(m_Timer)
    {
        m_Timer->stop();
        delete m_Timer;
    }
}

void reservewindow::on_TimerEvent()
{
    if(!m_ChargerA.IsBookingState() || m_ChargerA.VendorErrorCode != "")
    {
        // RFIDThread.~CRfidThread();
        if(pMsgWin ==NULL)
            StatusLedCtrl::instance().clear();    //关闭PCB红灯
        if(pCurWin ==this)  //开启充电时才会有设置pCurWin=本窗口
        {
            qWarning() << "RESERVE WINDOW: on_TimerEvent, not in booking state, close window";
            pCurWin =NULL;
            this->close();
        }
        else this->hide();
    }


    

    if(msg_Delay)
    {
        if (charger::reduceScreenTransitions())
            msg_Delay = 0; // do-not-wait
        else
            msg_Delay--;
        
        if(!msg_Delay)
        {
            if(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected())
            {
                qWarning() << "RESERVE WINDOW: reset RFID msg";
                ui->msg->setText("Please tap RFID card\n請拍RFID卡");
            }
            else
            {
                qWarning() << "RESERVE WINDOW: reset reserved msg";
                ui->msg->setText("This charger is reserved\n此充電器已預留");
            }
        }
    }

    // if(m_StayTimer) m_StayTimer--;

    // if(!this->isHidden())
    {
        // if(RFIDThread.GetStatus())
        //     ui->label_HW_status->setVisible(true);
        // else
        //     ui->label_HW_status->setVisible(false);
    }

    // no any cable connection, go to previous page, delte later
    // if((m_ChargerA.ConnType() == CCharger::ConnectorType::EvConnector
    //     || m_ChargerA.ConnType() == CCharger::ConnectorType::AC63A_NoLock)
    //     && (!(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected())))
    // {
    //     if((m_ChargerA.Command &_C_CHARGEMASK) !=_C_CHARGECOMMAND)
    //         on_pushButton_exit_clicked();
    // }
    // else
    // {
        if(m_Delay)
        {
            if (charger::reduceScreenTransitions())
                m_Delay = 0; // do-not-wait
            else
                m_Delay--;
            if(!m_Delay)
            {
                if(pMsgWin ==NULL)
                    StatusLedCtrl::instance().setState(StatusLedCtrl::State::purple);
                if(pCurWin ==this)
                {
                    // ui->secondaryResultLabel->setText("");     //test
                    CardNo.clear();
                    // RFIDThread.ReadCardAgain();
                    // setWinBg(RfidReqStatus::PresentCard, UserRequest::Authorize);
                }
                else
                {
                    if(this->windowTitle() =="STOP ERR")
                    {
                        CardNo.clear();
                        // RFIDThread.ReadCardAgain();
                        this->hide();
                        // setWinBg(RfidReqStatus::PresentCard, UserRequest::Authorize);
                    }
                    else
                    {
                        m_ChargerA.StopCharge();
                        m_ChargerA.UnLock();
                        this->close(); //停充后会在充电窗口按钮出错
                    }
                }
            }
        }
    // }
}

// void reservewindow::on_RFIDCard(QByteArray cardno)
// {
//     qWarning() << "RESERVE WINDOW: on_RFIDCard";
//     if(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected() && !pMsgWin)
//     {
//         qWarning() << "RESERVE WINDOW: RFID auth accepted";
//         // if(m_StayTimer) { m_StayTimer =5*2; RFIDThread.ReadCardAgain(); return; } //已读过的卡还未取开

//         CardNo.clear();
//         CardNo = cardno.toHex().toUpper();

//         if(m_ChargerA.ResIdTag ==CardNo)
//         {
//             if(pCurWin ==this)
//             {
//                 qWarning() << "RESERVE WINDOW: Start select charge mode";
//                 // RFIDThread.~CRfidThread();

//                 using namespace charger;

//                 m_ChargerA.IdTag =CardNo;
//                 switch (getChargingMode()) {
//                 case ChargeMode::UserSelect: //普通模式 设置定时
//                     pCurWin =new ChargingModeWin(this->parentWidget());
//                     pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
//                     pCurWin->show();
//                     this->close();
//                     break;
//                 default: //简易模式 自动充满
//                     m_ChargerA.StartChargeToFull();

//                     pCurWin =new WaittingWindow(this->parentWidget());
//                     pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
//                     pCurWin->show();
//                     this->close();
//                     qDebug()<<"Auto after RFID";
//                     break;
//                 }
//             }
//             else
//             {
//                 // setWinBg(RfidReqStatus::Success, UserRequest::StopCharge);
//                 if(m_Delay ==0) m_Delay =2;    //延时1"//4: 2"
//                 if(this->isHidden()) this->show();
//                 this->setWindowTitle("STOP");
//             }
//         }
//         else
//         {
//             qWarning() << "RESERVE WINDOW: incorrect RFID";
//             StatusLedCtrl::instance().setState(StatusLedCtrl::State::RFIDError);   //5
//             if(m_Delay ==0) m_Delay =10;    //延时5"

//             if(m_ChargerA.Status !=ocpp::ChargerStatus::Charging)
//             {
//                 qDebug() <<"CardNo:" <<CardNo;  //test
//                 // ui->secondaryResultLabel->setText(CardNo);     //test RFID Card ID
//                 // setWinBg(RfidReqStatus::Failed, UserRequest::Authorize);
//             }
//             else
//             {
//                 this->setWindowTitle("STOP ERR");
//                 // setWinBg(RfidReqStatus::Failed, UserRequest::StopCharge);
//                 if(this->isHidden()) this->show();
//             }

            
//             ui->msg->setText("Incorrect RFID Card\nRFID卡不正確");
//             if(msg_Delay ==0) msg_Delay =10;
//         }
//     }
//     else
//     {
//         qWarning() << "RESERVE WINDOW: Cable not plugin";
//         StatusLedCtrl::instance().setState(StatusLedCtrl::State::RFIDError);   //5
//             if(m_Delay ==0) m_Delay =10;    //延时5"

        
//         ui->msg->setText("Please Plug In Cable\n請插入電纜");
//         if(msg_Delay ==0) msg_Delay =10;
//     }
// }

void reservewindow::onChargeCommandAccepted()
{
    qWarning() << "reservewindow: Charge command accepted";
    rfidAuthorizer->stop();
    StatusLedCtrl::instance().clear();
    WaittingWindow *waitWin = new WaittingWindow(this->parentWidget());
    waitWin->setSeekApprovalBeforeCharge(false);
    waitWin->setWindowState(charger::Platform::instance().defaultWinState);
    waitWin->show();
    pCurWin->close();
    pCurWin = waitWin;
}

void reservewindow::startRfidAuthorizor()
{
    qWarning() << "reservewindow: Start RFID authorizor";
    if (!rfidAuthStarted) {
        rfidAuthorizer->start(ChargerRequestRfid::Purpose::BeginCharge, false);
        rfidAuthStarted = true;
    }
}

void reservewindow::onRequestAccepted()
{
    qWarning() << "reservewindow: Request accepted";
    rfidAuthorizer->stop();
    StatusLedCtrl::instance().clear();
    switch (charger::getChargingMode()) {
    case charger::ChargeMode::UserSelect:
        qWarning() << "reservewindow: Request accepted. User select";
        pCurWin = new ChargingModeImmediateWin(this->parentWidget());
        pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
        pCurWin->show();
        this->close();
        break;
    case charger::ChargeMode::ChargeToFull:
        qWarning() << "reservewindow: Request accepted. Charge to full";
        // Start Charging Now
        m_ChargerA.StartChargeToFull();
        pCurWin =new WaittingWindow(this->parentWidget());
        pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
        pCurWin->show();
        this->close();
        qDebug() <<"Auto in Mode";
        break;
    default:
        qWarning() << "reservewindow: Unexpected charging-mode value " << static_cast<int>(charger::getChargingMode());
        break;
    }
}

void reservewindow::onRequestCanceled()
{
    qWarning() << "reservewindow: Request canceled. Closing this window";
    StatusLedCtrl::instance().clear();
    Q_ASSERT(rfidAuthorizer);

    if (m_ChargerA.ConnType() == CCharger::ConnectorType::WallSocket) {
        // if it is 13A - this window will stay forever because disconnecting
        // cable will not be detected - and it will stay forever.
        if (rfidAuthStarted && rfidAuthorizer)
            rfidAuthorizer->stop();
        if (pCurWin == this) {
            pCurWin = nullptr;
            this->close();
        }
    }
    else {
        // Do nothing - RFID is the only way forward from this point on
    }
}
