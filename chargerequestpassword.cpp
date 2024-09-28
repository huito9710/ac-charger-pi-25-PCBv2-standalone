#include "chargerequestpassword.h"
#include "gvar.h"
#include "passwordmgr.h"
#include "api_function.h"
#include "chargingmodewin.h"
#include "waittingwindow.h"
#include "hostplatform.h"

ChargeRequestPassword::ChargeRequestPassword(QObject *parent) : QObject(parent)
{
    // Create Timer to Check Charger-Connection Status periodically
    switch (m_ChargerA.ConnType()) {
    case CCharger::ConnectorType::EvConnector:
    case CCharger::ConnectorType::AC63A_NoLock:
        connect(&chargerConnCheckTimer, SIGNAL(timeout()), this, SLOT(onTimerEvent()));
        chargerConnCheckTimer.start(timerPeriod);
        break;
    default:
        break;
    }
}

ChargeRequestPassword::~ChargeRequestPassword()
{
    chargerConnCheckTimer.stop();
    closePasswordWin();
}

void ChargeRequestPassword::onTimerEvent()
{
    if(!(m_ChargerA.IsCablePlugIn() && m_ChargerA.IsEVConnected()))
    {
        if((m_ChargerA.Command &_C_CHARGEMASK) !=_C_CHARGECOMMAND) {
            int hello = m_ChargerA.Command;
            qDebug() << "Charger Command set to: 0x" << hex <<  hello << endl;
            quit(false);
        }
    }
}

QMainWindow *ChargeRequestPassword::Start(QWidget *parent)
{
    passwordWin = new PassWordWin(parent);
    connect(passwordWin, &PassWordWin::passwordEntered, this, &ChargeRequestPassword::onPasswordEntered);
    connect(passwordWin, &PassWordWin::quitRequested, this, &ChargeRequestPassword::onPasswordCanceled);
    return passwordWin;
}

void ChargeRequestPassword::onPasswordEntered(QString userInput)
{
    QString itemvalue;

    if(isPasswordValid(userInput) && ((pCurWin == passwordWin) || (m_ChargerA.Password == userInput))) //增加原密码才能停止
    {
        if(pCurWin == passwordWin)
        {
            using namespace charger;
            m_ChargerA.Password = userInput;
            switch(getChargingMode()) {
            case ChargeMode::UserSelect: //普通模式 设置定时
                pCurWin =new ChargingModeWin(passwordWin->parentWidget());
                pCurWin->setWindowState(Qt::WindowFullScreen);
                pCurWin->show();
                closePasswordWin();
                break;
            default: //简易模式 自动充满
                m_ChargerA.ChargeTimeValue =100*60*60;  //100h
                m_ChargerA.RemainTime =m_ChargerA.ChargeTimeValue;
                m_ChargerA.StartCharge();

                pCurWin =new WaittingWindow(passwordWin->parentWidget());
                pCurWin->setWindowState(charger::Platform::instance().defaultWinState);
                pCurWin->show();
                closePasswordWin();
                qDebug()<<"Auto after password";
                //Waitting charging
                break;
            }
        }
        else // Frankie: How to reach this case?
        {
            m_ChargerA.StopCharge();
            m_ChargerA.UnLock();
            closePasswordWin();
        }
        quit(true);
    }
    else {
        passwordWin->RequestClear();
    }
}

void ChargeRequestPassword::onPasswordCanceled()
{
    closePasswordWin();
    quit(false);
}

bool ChargeRequestPassword::isPasswordValid(const QString &userInputCode)
{
    PasswordMgr passwordMgr;
    bool isValid = false;

    if (passwordMgr.IsPasswordPresent()) {
        if (userInputCode == passwordMgr.GetPassword().c_str()) {
            isValid = true;
        }
    }

    return isValid;
}

/*
 * This object will 'delete' itself because the callers are not expected to live long enough, based
 * on the current program design.
 */
void ChargeRequestPassword::quit(bool passwordAccepted)
{
    emit requestCompleted(passwordAccepted == true);
    delete this;
}
