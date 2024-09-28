#include "waittingwindow.h"
#include "ui_waittingwindow.h"
#include "gvar.h"
#include "./OCPP/ocppclient.h"
#include "hostplatform.h"
#include "chargerconfig.h"

WaittingWindow::WaittingWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WaittingWindow),
    connIcon(m_OcppClient),
    reqCentralServerBeforeCharge(false),
    quitRequested(false)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);

    connect(&cableMonitorTimer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
    cableMonitorTimer.start(cableMonitorInterval);

    updateWinBgStyleString(BgMessage::Connecting, charger::getCurrDisplayLang());
    this->setStyleSheet(winBgStyle);

    charger::ChargeMode chMode = charger::getCurrentChargingMode();
    switch (chMode) {
    case charger::ChargeMode::ChargeToFull:
    case charger::ChargeMode::UserSelect:
        if (charger::isStandaloneMode()) {
            // standalone-mode
            // TODO: prepareStandaloneModeAuthWin();
        }
        else{
            charger::PayMethod pay = charger::getPayMethod();
            switch (pay) {
            case charger::PayMethod::RFID:
                rfidAuthorizer = std::make_unique<ChargerRequestRfid>(parent);
                connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestAccepted, this, &WaittingWindow::onRfidAuthAccepted);
                connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestCanceled, this, &WaittingWindow::onRfidAuthCanceled);
                rfidAuthorizer->setStartTxRfid(m_ChargerA.IdTag);
                rfidAuthorizer->start(ChargerRequestRfid::Purpose::EndCharge, false); // listen to RFID card in the background
                break;
            default:
                break;
            }
        }
        break;
    default:
        // Do nothing
        break;
    }

    QString quitButtonStyle = QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1/PNG/Rect5.png);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir);
    ui->pushButton_exit->setStyleSheet(quitButtonStyle);

    if(charger::isHardKeySupportRequired())
    {
        ui->pushButton_exit->setCheckable(true);
        ui->pushButton_exit->setChecked(true);
    }
}

WaittingWindow::~WaittingWindow()
{
    delete ui;

    if(cableMonitorTimer.isActive())
        cableMonitorTimer.stop();
}

void WaittingWindow::showEvent(QShowEvent * event)
{
    Q_UNUSED(event);
    if (reqCentralServerBeforeCharge) {
        connect(&m_OcppClient, SIGNAL(OcppResponse(QString)), this, SLOT(on_OcppResponse(QString)));
        m_OcppClient.addDataTransferReq();
    }
}

void WaittingWindow::on_pushButton_exit_clicked()
{
    quitRequested = true;
    m_ChargerA.UnLock();
    m_ChargerA.ChargerIdle();
    if (rfidAuthorizer)
        rfidAuthorizer->stop();
    EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
    if (!reg.hasActiveTransaction()) {
        this->close();
        pCurWin =NULL;
    }
}

void WaittingWindow::onRfidAuthAccepted()
{
    // Stop charging
    this->on_pushButton_exit_clicked();
    // timer will monitor the subsequent change in charging-status.
    rfidAuthorizer->stop();
}

void WaittingWindow::onRfidAuthCanceled()
{
    // Do nothing and keep on waiting
    rfidAuthorizer->runBackground();
}

void WaittingWindow::on_TimerEvent()
{
    // Stay open after user-quit until there is no more active transaction.
    if (quitRequested) {
        EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
        if (!reg.hasActiveTransaction()) {
            this->close();
            pCurWin =NULL;
        }
        else {
            qDebug() << "Waiting for active transaction to close.";
        }
    }
    if((m_ChargerA.ConnType() == CCharger::ConnectorType::EvConnector
        || m_ChargerA.ConnType() == CCharger::ConnectorType::AC63A_NoLock)
        && (!(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected())))
    {
        on_pushButton_exit_clicked();
    }
    //else if(this->windowTitle() =="Net-RFID") or now is reqCentralServerBeforeCharge
    else if((m_ChargerA.Command &_C_CHARGEMASK) ==_C_CHARGEIDLE)
    {
        showRequestFailedMsg();
    }
}

void WaittingWindow::showRequestFailedMsg()
{
    using namespace charger;
    if(this->windowTitle() !="Abort")       //当充电命令已被取消, 标题不是Abort时, 更新界面为取消图片
    {
        this->setWindowTitle("Abort");
        updateWinBgStyleString(BgMessage::RequestFailed, charger::getCurrDisplayLang());
        this->setStyleSheet(winBgStyle);
    }
}

void WaittingWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() ==Qt::Key_Return)
    {
        on_pushButton_exit_clicked();
    }
}

void WaittingWindow::on_OcppResponse(QString datatransfer)
{
    //[临时OCPP标准试用
    m_ChargerA.StartCharge();   //本机发起充电;
    return;
    //]

    if(datatransfer =="TimeValue Accepted")
    {
        //TODO:等待平台启动充电 或本机发起充电 (当前没限制等多久)
        m_ChargerA.StartCharge();   //本机发起充电;
    }
    else //平台拒绝
    {
        qDebug() <<"平台拒绝DataTransfer";
        showRequestFailedMsg();
    }
}

void WaittingWindow::updateWinBgStyleString(enum BgMessage msg, charger::DisplayLang lang)
{
    using namespace charger;

    QString langStr;

    switch (lang)
    {
    case DisplayLang::Chinese:
        langStr = "ZH"; break;
    case DisplayLang::English:
        langStr = "EN"; break;
    default:
        Q_ASSERT(false);
    }

    QString jpgFileName;
    switch (msg)
    {
    case BgMessage::Connecting:
        jpgFileName = "WaittingWin"; break;
    case BgMessage::RequestFailed:
        jpgFileName = "AbortTips"; break;
    default:
        Q_ASSERT(false);
    }

    winBgStyle = QString("QWidget#centralwidget {background-image: url(%1%2.%3.jpg);}").arg(charger::Platform::instance().guiImageFileBaseDir).arg(jpgFileName).arg(langStr);

}
