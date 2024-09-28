#include "paymentmethodwindow.h"
#include "ui_paymentmethodwindow.h"
#include "octopuswindow.h"
#include "wapwindow.h"
#include "chargingmodeimmediatewin.h"
#include "waittingwindow.h"
#include "ccharger.h"
#include "api_function.h"
#include "const.h"
#include "gvar.h"
#include "chargerconfig.h"
#include "./OCPP/ocppclient.h"
#include "hostplatform.h"
#include <iostream>
#include <iomanip>
#include "qrcodegen/QrCode.hpp"
#include "statusledctrl.h"

static void freePixBuf(void *buf);
static void paintQr(const qrcodegen::QrCode &qr, QLabel *label);

PaymentMethodWindow::PaymentMethodWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PaymentMethodWindow),
    rfidAuthStarted(false),
    bgMode(false),
    connIcon(m_OcppClient)
{
    qWarning() << "PAYMENT METHOD WINDOW: init";
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);

    initPayMethod();
    updateStyleStrs(payMethod, charger::getCurrDisplayLang());
    this->setStyleSheet(winBgStyle);
    QString chargerNumStr;
    if (GetSetting("CHARGER NO", chargerNumStr)) {
        //int chargerNum = chargerNumStr.toInt();
        //displayChargerNum(payMethod, chargerNum);
        displayChargerNum(payMethod, chargerNumStr);
    }
    std::string chargerSerNumStr = charger::getChargePointSerialNum();
    displaySerialNum(payMethod, chargerSerNumStr.c_str());
    displayChargerName(payMethod, QString::fromStdString(charger::getChargerName()));
    qDebug() <<winBgStyle;


    connect(&chargerConnCheckTimer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
    chargerConnCheckTimer.start(500);

    connect (&m_ChargerA, &CCharger::chargeCommandAccepted, this, &PaymentMethodWindow::onChargeCommandAccepted);

    if(payMethod == charger::PayMethod::RFID)
    {
        rfidAuthorizer = std::make_unique<ChargerRequestRfid>(this);
        connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestAccepted, this, &PaymentMethodWindow::onRequestAccepted);
        connect(rfidAuthorizer.get(), &ChargerRequestRfid::requestCanceled, this, &PaymentMethodWindow::onRequestCanceled);
    }

    if (charger::isHardKeySupportRequired())
        ui->pushButton->setChecked(true);
}

PaymentMethodWindow::~PaymentMethodWindow()
{
    delete ui;
    chargerConnCheckTimer.stop();
}

void PaymentMethodWindow::initPayMethod()
{
    payMethod = charger::getPayMethod();
}

void PaymentMethodWindow::updateStyleStrs(charger::PayMethod payMethod, charger::DisplayLang lang)
{
    using namespace charger;

    QString langInFn;
    switch (lang) {
    case DisplayLang::Chinese:
    case DisplayLang::SimplifiedChinese:
        langInFn = "ZH"; break;
    case DisplayLang::English: langInFn = "EN"; break;
    }

    QString fileNameBase;
    switch (payMethod) {
    case PayMethod::App: fileNameBase = "App"; break;
    case PayMethod::Octopus: fileNameBase = "Octopus"; break;
    case PayMethod::App_Octopus: fileNameBase = "AppOctopus"; break;
    default:
        // Do nothing for RFID - use color
        break;
    }

    if (fileNameBase.isEmpty())
        winBgStyle = QString("QWidget#centralwidget{background-color: RGB(128,128,128);}");
    else
        winBgStyle = QString("QWidget#centralwidget{background-image: url(%1/%2.%3.jpg);}").arg(Platform::instance().guiImageFileBaseDir).arg(fileNameBase).arg(langInFn);
}

//void PaymentMethodWindow::displayChargerNum(charger::PayMethod payMethod, int chargerNum)
void PaymentMethodWindow::displayChargerNum(charger::PayMethod payMethod, QString chargerNumStr)
{
    qWarning() << "PAYMENT METHOD WINDOW: Display charger number";
    // make it a 3-digit number, with leading zeros
    //std::stringstream ss;
    //ss << std::setfill('0') << std::setw(3) << chargerNum;
    using namespace charger;
    // hide all labels
    ui->chgrNumLabelLg->setVisible(false);
    ui->chgrNumLabelSm->setVisible(false);
    switch (payMethod) {
    case PayMethod::Octopus:
        ui->chgrNumLabelLg->setVisible(true);
        //ui->chgrNumLabelLg->setText(ss.str().c_str());
        chgrNumLabelLgStyle = QString("qproperty-alignment: AlignCenter; color:white; font-family: Piboto, Calibri, sans-serif; font-size: 75px");
        ui->chgrNumLabelLg->setStyleSheet(chgrNumLabelLgStyle);
        ui->chgrNumLabelLg->setText(chargerNumStr);
        break;
    case PayMethod::App_Octopus:
        ui->chgrNumLabelSm->setVisible(true);
        //ui->chgrNumLabelSm->setText(ss.str().c_str());
        chgrNumLabelSmStyle = QString("qproperty-alignment: AlignCenter; color:white; font-family: Piboto, Calibri, sans-serif; font-size: 35px");
        ui->chgrNumLabelSm->setStyleSheet(chgrNumLabelSmStyle);
        ui->chgrNumLabelSm->setText(chargerNumStr);
    default:
        break;
    }
}

static void freePixBuf(void *buf)
{
    if (buf) {
        uchar *pixBuf = static_cast<uchar *>(buf);
        delete [] pixBuf;
    }
}

static void paintQr(const qrcodegen::QrCode &qr, QLabel *label)
{
#if false
    //56 x 56
    int rowStride = 7;
    int numRows = 7*8;
    uchar *buffer = new uchar[numRows * rowStride];
    // Left white (1), Right-half black (0)
    memset(buffer, 0, (rowStride * numRows));
    uchar *line = buffer;
    for (int r = 0; r < numRows; r++) {
        uchar *p = line;
        for (int x = 0; x < ((8 * rowStride) / 2);) {
            int bit = x % 8;
            *p |= (1 << bit); // LSB
            x++;
            if (x%8 == 0)
                p++;
        }
        line += rowStride;
    }
#else
    int numRows = qr.getSize();
    int w = numRows;
    int rowStride = (w+7) / 8;
    uchar *buffer = new uchar[numRows * rowStride];
    // preset as Black pixels first
    memset(buffer, 0, numRows * rowStride);
    uchar *line = buffer;
    for (int r = 0; r < numRows; r++) {
        uchar *p = line;
        for (int x = 0; x < w;) {
            int bit = x % 8;
            if (!qr.getModule(x,r)) {
                // white
                *p |= (1 << bit); // LSB
            }
            x++;
            if ((x%8) == 0) {
                p++;
            }
        }
        line += rowStride;
    }
#endif
    label->setPixmap(QPixmap::fromImage(QImage(buffer, w, numRows, rowStride, QImage::Format_MonoLSB, freePixBuf, buffer)).scaledToHeight(label->height() - (2*PaymentMethodWindow::qrCodeImageBorderWidth)));
}

void PaymentMethodWindow::displayChargerName(charger::PayMethod payMethod, QString chargerName)
{
    qWarning() << "PAYMENT METHOD WINDOW: Display charger name";
    // hide everything first
    ui->qrLabelLg->setVisible(false);
    ui->qrLabelSm->setVisible(false);
    using namespace qrcodegen;
    const QrCode::Ecc errCorLvl = QrCode::Ecc::LOW;
    const qrcodegen::QrCode qr = QrCode::encodeText(chargerName.toUtf8(), errCorLvl);
    using namespace charger;
    switch (payMethod) {
    case PayMethod::App:
        paintQr(qr, ui->qrLabelLg);
        ui->qrLabelLg->setVisible(true);
        break;
    case PayMethod::App_Octopus:
        paintQr(qr, ui->qrLabelSm);
        ui->qrLabelSm->setVisible(true);
        break;
    default:
        break;
    }
}

void PaymentMethodWindow::displaySerialNum(charger::PayMethod payMethod, QString serialNum)
{
    qWarning() << "PAYMENT METHOD WINDOW: Display serial number";
    // hide everything first
    ui->serNumLabelLg->setVisible(false);
    ui->serNumLabelSm->setVisible(false);
    using namespace charger;
    switch (payMethod) {
    case PayMethod::App:
        ui->serNumLabelLg->setText("No. " + serialNum);
        ui->serNumLabelLg->setVisible(true);
        break;
    case PayMethod::App_Octopus:
        ui->serNumLabelSm->setText("No. " + serialNum);
        ui->serNumLabelSm->setVisible(true);
        break;
    default:
        break;
    }
}

void PaymentMethodWindow::onChargeCommandAccepted()
{
    qWarning() << "PAYMENT METHOD WINDOW: Charge command accepted";
    WaittingWindow *waitWin = new WaittingWindow(this->parentWidget());
    waitWin->setSeekApprovalBeforeCharge(false);
    waitWin->setWindowState(charger::Platform::instance().defaultWinState);
    waitWin->show();
    if (!bgMode)
        Q_ASSERT(pCurWin == this);
    pCurWin->close();
    pCurWin = waitWin;
}

void PaymentMethodWindow::on_pushButton_clicked()
{ //HOME:
    qWarning() << "PAYMENT METHOD WINDOW: Closing this window";
    if(pMsgWin ==NULL)
        StatusLedCtrl::instance().clear();    //关闭PCB红灯
    m_ChargerA.UnLock();
    m_ChargerA.ChargerIdle();
    this->close();
    if (bgMode)
        emit bgWindowClosed();
    pCurWin =NULL;
}

void PaymentMethodWindow::on_pushButton_2_clicked()
{
    qWarning() << "PAYMENT METHOD WINDOW: push button 2 clicked";
    //测试开启充电:
    qDebug() <<"start charge...";
    m_ChargerA.ChargeTimeValue =300;
    m_ChargerA.RemainTime =m_ChargerA.ChargeTimeValue;
    m_ChargerA.StartCharge();
    return;
    //----------------

    pCurWin =new OctopusWindow(this->parentWidget());
    pCurWin->setWindowState(Qt::WindowFullScreen);
    pCurWin->show();

    this->close();
}

void PaymentMethodWindow::on_pushButton_3_clicked()
{
    qWarning() << "PAYMENT METHOD WINDOW: push button 3 clicked";
    pCurWin =new WapWindow(this->parentWidget());
    pCurWin->setWindowState(Qt::WindowFullScreen);
    pCurWin->show();

    this->close();
}

void PaymentMethodWindow::on_TimerEvent()
{
    if((m_ChargerA.ConnType() == CCharger::ConnectorType::EvConnector
        || m_ChargerA.ConnType() == CCharger::ConnectorType::AC63A_NoLock)
        &&(!(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected())))
    {
        if((m_ChargerA.Command &_C_CHARGEMASK) !=_C_CHARGECOMMAND)
            on_pushButton_clicked();
    }
}

void PaymentMethodWindow::startRfidAuthorizor()
{
    qWarning() << "PAYMENT METHOD WINDOW: Start RFID authorizor";
    if (!rfidAuthStarted) {
        rfidAuthorizer->start(ChargerRequestRfid::Purpose::BeginCharge, bgMode? false: true);
        rfidAuthStarted = true;
    }
}

void PaymentMethodWindow::showEvent(QShowEvent *)
{
    if (payMethod == charger::PayMethod::RFID) {
        // use a singleshot timer to queue this action _after_ the Payment window
        // is actually show. This prevents the case where the RFID Window
        // gets under the PaymentWindow
        QTimer::singleShot(0, this, &PaymentMethodWindow::startRfidAuthorizor);
    }
}

void PaymentMethodWindow::runInBackground()
{
    bgMode = true;
    if (payMethod == charger::PayMethod::RFID) {
        // use a singleshot timer to queue this action _after_ the Payment window
        // is actually show. This prevents the case where the RFID Window
        // gets under the PaymentWindow
        QTimer::singleShot(0, this, &PaymentMethodWindow::startRfidAuthorizor);
    }
}

void PaymentMethodWindow::keyPressEvent(QKeyEvent *event)
{
    qWarning() << "PAYMENT METHOD WINDOW: Key press event";
    if(event->key() ==Qt::Key_Return)
    {
        on_pushButton_clicked();
    }
}

void PaymentMethodWindow::onRequestAccepted()
{
    qWarning() << "PAYMENT METHOD WINDOW: Request accepted";
    switch (charger::getChargingMode()) {
    case charger::ChargeMode::UserSelect:
        qWarning() << "PAYMENT METHOD WINDOW: Request accepted. User select";
        pCurWin = new ChargingModeImmediateWin(this->parentWidget());
        pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
        pCurWin->show();
        this->close();
        if (bgMode)
            emit bgWindowClosed();
        break;
    case charger::ChargeMode::ChargeToFull:
        qWarning() << "PAYMENT METHOD WINDOW: Request accepted. Charge to full";
        // Start Charging Now
        m_ChargerA.StartChargeToFull();
        pCurWin =new WaittingWindow(this->parentWidget());
        pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
        pCurWin->show();
        this->close();
        if (bgMode)
            emit bgWindowClosed();
        qDebug() <<"Auto in Mode";
        break;
    default:
        qWarning() << "PaymentMethodWindow: Unexpected charging-mode value " << static_cast<int>(charger::getChargingMode());
        break;
    }
}

void PaymentMethodWindow::onRequestCanceled()
{
    qWarning() << "PAYMENT METHOD WINDOW: Request canceled. Closing this window";
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


