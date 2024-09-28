#include "sockettype.h"
#include "ui_sockettype.h"
#include "cablewin.h"
#include "ccharger.h"
#include "paymentmethodwindow.h"
#include "chargerequestpassword.h"
#include "rfidwindow.h"
#include "gvar.h"
#include "hostplatform.h"
#include "passwordupdater.h"
#include "waittingwindow.h"

SocketType::SocketType(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SocketType),
    connIcon(m_OcppClient),
    connSelCheckedButtonStyle(QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1PNG/Rect1.png);}").arg(charger::Platform::instance().guiImageFileBaseDir)),
    langChangeCheckedButtonStyle(QString("QPushButton{background:transparent; border-image:none;} QPushButton:checked{border-image: url(%1PNG/Rect2.png);}").arg(charger::Platform::instance().guiImageFileBaseDir)),
    settingsCheckedButtonStyle(QString("QPushButton{background:transparent; border-image:url(%1PNG/settingsSmallButton.png); image:none;} QPushButton:checked{image: url(%1PNG/Rect2.png);}").arg(charger::Platform::instance().guiImageFileBaseDir))
{
    qWarning() << "SOCKETTYPE: init";
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);

    connect(&cableMonitorTimer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
    switch (m_ChargerA.ConnType()) {
    case CCharger::ConnectorType::EvConnector:
    case CCharger::ConnectorType::AC63A_NoLock:
        cableMonitorTimer.start(200);
        break;
    default:
        break;
    }

    initStyleStrings(charger::getCurrDisplayLang());
    this->setStyleSheet(socketTypeWinBgStyle);

    // Set the images used in button at 'checked' state (cross-platform)
    ui->pushButton_2->setStyleSheet(connSelCheckedButtonStyle);
    ui->pushButton_3->setStyleSheet(connSelCheckedButtonStyle);
    ui->langChangeButton->setStyleSheet(langChangeCheckedButtonStyle);
    // settings button (password change)
    ui->passwordChangeButton->setStyleSheet(settingsCheckedButtonStyle);

    // only make it visible in standalone-mode
    ui->passwordChangeButton->setVisible(charger::passwordAuthEnabled());


    if(charger::isHardKeySupportRequired())
    {
        ui->pushButton_2->setCheckable(true);
        ui->pushButton_3->setCheckable(true);
        ui->langChangeButton->setCheckable(true);
        ui->passwordChangeButton->setCheckable(charger::passwordAuthEnabled());

        ui->pushButton_3->setChecked(true);
    }

    //on_TimerEvent();  //造成pCurWin错误
}

SocketType::~SocketType()
{
    delete ui;
    if(cableMonitorTimer.isActive())
        cableMonitorTimer.stop();
}

void SocketType::initStyleStrings(charger::DisplayLang lang)
{
    using namespace charger;
    QString langInFn;
    switch (lang)
    {
    case DisplayLang::English:
        langInFn = "EN";
        break;
    case DisplayLang::Chinese:
    case DisplayLang::SimplifiedChinese:
        langInFn = "ZH";
        break;
    }
    socketTypeWinBgStyle = QString("QWidget#centralwidget {background-image: url(%1socketTypeWin.%2.jpg);}").arg(charger::Platform::instance().guiImageFileBaseDir).arg(langInFn);
}

void SocketType::on_pushButton_2_clicked()
{
    qWarning() << "SOCKETTYPE: Closing this window";
    EvChargingTransactionRegistry &reg = EvChargingTransactionRegistry::instance();
    Q_ASSERT(!reg.hasActiveTransaction());
    m_ChargerA.SetConnType(CCharger::ConnectorType::WallSocket); //13A
    if (charger::isStandaloneMode()) {
        using namespace charger;
        switch (getChargingMode()) {
        case ChargeMode::NoAuthChargeToFull:
            // Starts charging?
            m_ChargerA.StartChargeToFull();
            pCurWin =new WaittingWindow(this->parentWidget());
            break;
        default:
            if(charger::getRfidSupported()) {
                pCurWin =new RFIDWindow(this->parentWidget());
            }
            else {
                ChargeRequestPassword *reqPassword = new ChargeRequestPassword();
                // reqPassword will de-allocate itself when the there is no more work to be done
                // (lack of reference to reqPassword object)
                pCurWin = reqPassword->Start(this->parentWidget());
            }
        }
    }
    else {
         pCurWin =new PaymentMethodWindow(this->parentWidget());
    }

    pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
    pCurWin->show();
    this->close();
}

void SocketType::on_pushButton_3_clicked()
{
    qWarning() << "SOCKETTYPE: push button 3 clicked";
    m_ChargerA.SetConnType(CCharger::ConnectorType::EvConnector);
    pCurWin =new CableWin(this->parentWidget());
    pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
    pCurWin->show();
    this->close();
}

void SocketType::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        initStyleStrings(charger::getCurrDisplayLang());
        this->setStyleSheet(socketTypeWinBgStyle);
    }
}

void SocketType::on_langChangeButton_clicked()
{ //language:
    qWarning() << "SOCKETTYPE: Language change button clicked";
    charger::toggleDisplayLang();
    // refer to the windows ChangeEvent() handler
}

void SocketType::on_TimerEvent()
{
    if(m_ChargerA.IsCablePlugIn() && !pMsgWin) on_pushButton_3_clicked();
}

void SocketType::keyPressEvent(QKeyEvent *event)
{
    qWarning() << "SOCKETTYPE: Key press event";
    switch (event->key()) {
    case Qt::Key_Right:
    case Qt::Key_PageDown:
        // PB2 -> PB3 -> PB4 -> PasswordChange -> PB2
        if(ui->pushButton_2->isChecked())
        {
            ui->pushButton_2->setChecked(false);
            ui->pushButton_3->setChecked(true);
        }
        else if(ui->pushButton_3->isChecked())
        {
            ui->pushButton_3->setChecked(false);
            ui->langChangeButton->setChecked(true);
        }
        else if(ui->langChangeButton->isChecked())
        {
            ui->langChangeButton->setChecked(false);
            if (ui->passwordChangeButton->isVisible())
                ui->passwordChangeButton->setChecked(true);
            else
                ui->pushButton_2->setChecked(true);
        }
        else if(ui->passwordChangeButton->isChecked())
        {
            ui->passwordChangeButton->setChecked(false);
            ui->pushButton_2->setChecked(true);
        }
        break;
    case Qt::Key_Return:
        if(ui->pushButton_2->isChecked())
            on_pushButton_2_clicked();
        else if(ui->pushButton_3->isChecked())
            on_pushButton_3_clicked();
        else if(ui->langChangeButton->isChecked())
            on_langChangeButton_clicked();
        else if(ui->passwordChangeButton->isChecked())
            on_passwordChangeButton_clicked();
        break;
    default:
        qWarning() << "Unhandled key event (" << event->key() << ").";
        break;
    }
}

void SocketType::on_passwordChangeButton_clicked()
{
    qWarning() << "SOCKETTYPE: Password change button clicked";
    PasswordUpdater *pwUpdater = new PasswordUpdater(nullptr);
    pwUpdater->start(); // object will free itself when done.
}

