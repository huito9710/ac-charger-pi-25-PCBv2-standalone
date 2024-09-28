#include "messagewindow.h"
#include "ui_messagewindow.h"
#include "ccharger.h"
#include "hostplatform.h"
#include "chargerconfig.h"
#include "gvar.h"
#include "statusledctrl.h"

using namespace charger;

MessageWindow::MessageWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MessageWindow),
    connIcon(m_OcppClient)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);

    pMsgWin =this;

    ui->label_msg->setText("");
    if(m_ChargerA.VendorErrorCode =="")
    {
        ui->label_errcode->setText("");
        ui->pushButton_2->setEnabled(false);
    }
    else
    {
        //ui->label_errcode->setText(m_ChargerA.VendorErrorCode);
        UpdateMessage();
    }

    if(m_ChargerA.VendorErrorCode != "")
    {
        qWarning() << "message window, error, change LED light";
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashingred);
    }
}

MessageWindow::~MessageWindow()
{
    delete ui;
    pMsgWin =NULL;
    m_ChargerA.OldMsg ="";
    if(m_ChargerA.IsPreparingState() || m_ChargerA.IsFinishing())
    {
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::yellow);
    }
    else if(m_ChargerA.IsChargeState() || m_ChargerA.IsSuspendedEVorEVSE())
    {
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashinggreen);
    }
    else if(m_ChargerA.IsIdleState())
    {
//        if(charger::GetQueueingMode())
//        {
//            StatusLedCtrl::instance().setState(StatusLedCtrl::State::blue);
//        }
//        else
//        {
            StatusLedCtrl::instance().setState(StatusLedCtrl::State::green);
//        }
    }
}

void MessageWindow::on_pushButton_2_clicked()
{
    this->close();
    pMsgWin =NULL;
}

void MessageWindow::UpdateMessage()
{
    if(ui->label_errcode->text() !="")
    {
        if(m_ChargerA.VendorErrorCode !="")
        {
            if(m_ChargerA.VendorErrorCode !=m_ChargerA.OldMsg) m_ChargerA.SaveMsgRecord(m_ChargerA.VendorErrorCode);
            ui->label_errcode->setText(m_ChargerA.VendorErrorCode);
        }
        else on_pushButton_2_clicked();
    }
}

void MessageWindow::enableCloseButton()
{
    ui->pushButton_2->setEnabled(true);
}

void MessageWindow::showEvent(QShowEvent * event)
{
    //qDebug() <<this->windowTitle();
    if(this->windowTitle() =="") this->setWindowTitle("messageWin");
    QString windowStyleSheet = QString("QWidget#centralwidget {background-image: url(%1%2.%3.jpg);}").arg(Platform::instance().guiImageFileBaseDir)
            .arg(this->windowTitle()).arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "EN" : "ZH");
    this->setStyleSheet(windowStyleSheet);

    if(windowTitle() =="P1.NET_OFF")
    {
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::flashingred);
        m_ChargerA.SaveMsgRecord("Network error");
    }
    else if(windowTitle() =="P7.LOCK")
    {
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::purple);
    }
    else if(windowTitle() =="P7.UNLOCK")
    {
        StatusLedCtrl::instance().setState(StatusLedCtrl::State::purple);
    }

    Q_UNUSED(event);
}
