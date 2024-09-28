#include <QKeyEvent>
#include <QTimer>
#include "rfidreaderwindow.h"
#include "ui_rfidwindow.h"
#include "hostplatform.h"
#include "gvar.h"
#include "chargerconfig.h"
#include "statusledctrl.h"

RfidReaderWindow::RfidReaderWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RFIDWindow),
    succLabelStyle("QLabel{background-color: none; color: green; font-family: Piboto, Calibri, sans-serif; font-size: 22px;}"),
    failLabelStyle("QLabel{background-color: none; color: red; font-family: Piboto, Calibri, sans-serif; font-size: 22px;}"),
    purpose(Purpose::StartCharge),
    st(State::PresentCard),
    connIcon(m_OcppClient)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);
    if (charger::isHardKeySupportRequired()) {
        ui->pushButton_exit->setCheckable(true);
        ui->pushButton_exit->setChecked(true);
    }

    // window background image
    this->setStyleSheet(QString("QWidget#centralwidget {background-image: url(%1RFID_ReadCard.jpg);}").arg(charger::Platform::instance().guiImageFileBaseDir));

    // text label fonts
    QString titleLabelStyle = QString("QLabel{background-color: none; color: white; font-family: Piboto, Calibri, sans-serif; font-size: 24px;}");
    ui->titleLabel->setStyleSheet(titleLabelStyle);
    ui->secondaryResultLabel->setStyleSheet(failLabelStyle);

    connect(&readerThrd, &CRfidThread::RFIDCard, this, &RfidReaderWindow::onRFIDCard);
    //readerThrd.StartSlave(charger::Platform::instance().rfidSerialPortName, ReaderSerialPortRdTimeout);
    readerThrd.StartSlave(ReaderSerialPortRdTimeout);

    // Show Quit Button
    QString exitButtonStyle = QString("QPushButton{background:transparent; border-image:url(%1PNG/quit.%2.png); image:none;} QPushButton:checked{image: url(%1PNG/Rect2.png);}")
            .arg(charger::Platform::instance().guiImageFileBaseDir)
            .arg(charger::getCurrDisplayLang() == charger::DisplayLang::Chinese? "ZH" : "EN");
    ui->pushButton_exit->setStyleSheet(exitButtonStyle);
    ui->pushButton_exit->setVisible(true);
}

RfidReaderWindow::~RfidReaderWindow()
{
    delete ui;
}

void RfidReaderWindow::displayMessage(Message msg, bool autoClose)
{
    displayMessage(msg, RejectReason::Nil, autoClose);
}

void RfidReaderWindow::displayMessage(Message msg, RejectReason rejReason, bool autoClose)
{
    updateTitleLabel(msg);

    // update primary message
    switch (msg) {
        case Message::ServerOffline:
            ui->primaryResultLabel->setStyleSheet(failLabelStyle);
            ui->primaryResultLabel->setText(tr("Server temporarily offline. Please try again later."));
            break;
        case Message::StopRejectedTryAgain:
        case Message::StartRejectedTryAgain:
            ui->primaryResultLabel->setStyleSheet(failLabelStyle);
            ui->primaryResultLabel->setText(tr("RFID card error, please change the card and try again!"));
            break;
        case Message::StopTxnCardNotMatch:
            ui->primaryResultLabel->setStyleSheet(failLabelStyle);
            ui->primaryResultLabel->setText(tr("Please present the Same card that was used to Start Charging!"));
            break;
        default:
            ui->primaryResultLabel->clear();
        break;
    }

    // updates secondary message
    switch (rejReason) {
    case RejectReason::Expired: ui->secondaryResultLabel->setText(tr("Card is Expired.")); break;
    case RejectReason::BlackListed: ui->secondaryResultLabel->setText(tr("Card is Blacklisted.")); break;
    case RejectReason::TempBlocked: ui->secondaryResultLabel->setText(tr("Card is Temporarily Blocked.")); break;
    default:
        ui->secondaryResultLabel->clear();
        break;
    }

    if (autoClose) {
        st = State::RetryMessage;
        // set up one-shot timer
        if (charger::reduceScreenTransitions())
            QTimer::singleShot(ReducedAutoCloseMessagePeriod, this, SLOT(closeTemporaryMessage()));
        else
            QTimer::singleShot(AutoCloseMessagePeriod, this, SLOT(closeTemporaryMessage()));
    }
}

void RfidReaderWindow::updateTitleLabel(Message msg)
{
    if (msg == Message::AuthStarted) {
        ui->titleLabel->setText(tr("Authenticating, please wait..."));
    }
    else {
        // Update title message based on purpose
        switch (purpose) {
        case Purpose::StartCharge:
            ui->titleLabel->setText(tr("Tap RFID Card to Begin Charging"));
            break;
        case Purpose::StopCharge:
            ui->titleLabel->setText(tr("Tap RFID Card to Stop Charging"));
            break;
        case Purpose::SuspendCharge:
            ui->titleLabel->setText(tr("Tap RFID Card to Suspend Charging"));
            break;
        default: // Placeholder
            ui->titleLabel->setText(tr("Tap RFID Card to Confirm"));
            break;
        }
    }
}

void RfidReaderWindow::askForCard(Purpose purpose)
{
    this->purpose = purpose;

    // Reader control
    switch (st) {
    case State::PresentCard:
        readerThrd.ReadCard();
        break;
    case State::PresentCardAgain:
    case State::CardCaptured:
        readerThrd.ReadCardAgain();
        break;
    default:
        throw std::runtime_error("unexpected request to read card at this state!");
        break;
    }

    // Update title message based on purpose
   updateTitleLabel(Message::Nil);
    // Clear previous message
    ui->primaryResultLabel->clear();
    // Clear previous error message when asking for card.
    ui->secondaryResultLabel->clear();
    StatusLedCtrl &ledCtrl = StatusLedCtrl::instance();
    if (ledCtrl.getState() == StatusLedCtrl::State::flashingred)
        ledCtrl.clear();
}

void RfidReaderWindow::closeTemporaryMessage()
{
    switch (st) {
    case State::RetryMessage:
        st = State::PresentCardAgain;
        askForCard(purpose);
        break;
    default:
        throw "unexpected call to closeTemporaryMessage() at this state!";

    }
}

void RfidReaderWindow::onRFIDCard(const QByteArray cardUid)
{
    st = State::CardCaptured;
    emit readerRespond(ReaderStatus::CardReadOK, cardUid);
}

void RfidReaderWindow::requestClose()
{
    // TBD: cancel-read
    this->close();
}

void RfidReaderWindow::on_pushButton_exit_clicked()
{
    // emit signal that user request cancel read-card
    emit readerRespond(ReaderStatus::UserCancel);
}


void RfidReaderWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() ==Qt::Key_Return)
        on_pushButton_exit_clicked();
}
/*
void RfidReaderWindow::updateStyleStrs(Message msg, charger::DisplayLang lang)
{
    using namespace charger;
    QString langInFn;
    switch (lang) {
    case DisplayLang::Chinese:
    case DisplayLang::SimplifiedChinese:
        langInFn = "ZH"; break;
    case DisplayLang::English: langInFn = "EN"; break;
    }

    QString fileBaseName;
    switch (st) {
    case State::PresentCard:
    case State::PresentCardAgain:
        fileBaseName = "RFID_Start";
        break;
    case State::CardCaptured:
        switch (msg) {
        case Message::AuthStarted:
            fileBaseName = "RFID_Authorizing";
            break;
        default:
            throw std::runtime_error("Unhandled message type to display at RfidReaderWindow");
        }
        break;
    case State::RetryMessage:
        switch (msg) {
        case Message::StopRejectedTryAgain: fileBaseName = "RFID_StopErr"; break;
        case Message::StartRejectedTryAgain: fileBaseName = "RFID_StartErr"; break;
        default:
            throw std::runtime_error("Unhandled message type to display at RfidReaderWindow");
        }
    }

    winBgStyle = QString("QWidget#centralwidget {background-image: url(%1%2.%3.jpg);}").arg(Platform::instance().guiImageFileBaseDir).arg(fileBaseName).arg(langInFn);
    messageLabelStyle = QString("QLabel{background-color: none; color: red; font-family: Arial, Helvetica, sans-serif;}");
}
*/
