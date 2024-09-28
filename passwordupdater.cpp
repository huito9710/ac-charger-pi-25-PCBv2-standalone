#include "passwordupdater.h"
#include "messagewindow.h"
#include "gvar.h"
#include "hostplatform.h"

PasswordUpdater::PasswordUpdater(QObject *parent) :
    QObject(parent), state(State::Idle), passwordInputWin(nullptr)
{
    displayMsgs = {
                    {DisplayMsg::CurrentPasswordError, "CurrentPasswordWrong"},
                    {DisplayMsg::NewPasswordNotMatch, "NewPasswordMismatch"},
                    {DisplayMsg::PasswordUpdated, "PasswordUpdated"},
                  };
}
PasswordUpdater::~PasswordUpdater()
{
    // clean up
    if (passwordInputWin) {
        passwordInputWin->RequestClose();
        passwordInputWin = nullptr;
    }
}

void PasswordUpdater::start()
{
    if (state != State::Idle) {
      throw "PasswordUpdater: Unexpected State!";
    }

    // Load password from File
    if (passwordMgr.IsPasswordPresent()) {
        // Ask user to enter password if exists
        state = State::RequestCurrentPassword;
        showPasswordInputWin(PassWordWin::Prompt::EnterCurrentPassword);
    }
    else {
        // Ask user to enter new password
        state = State::RequestNewPassword;
        showPasswordInputWin(PassWordWin::Prompt::EnterNewPassword);
    }
}

void PasswordUpdater::showPasswordInputWin(PassWordWin::Prompt prompt)
{
    if (passwordInputWin == nullptr) {
        passwordInputWin = new PassWordWin(nullptr);
        passwordInputWin->SetPrompt(prompt);
        connect(passwordInputWin, &PassWordWin::passwordEntered, this, &PasswordUpdater::onPasswordEntered);
        connect(passwordInputWin, &PassWordWin::quitRequested, this, &PasswordUpdater::onPasswordCanceled);
    }
    passwordInputWin->setWindowState(charger::Platform::instance().defaultWinState);
    passwordInputWin->show();
}

void PasswordUpdater::showMessage(QString message)
{
    m_ChargerA.VendorErrorCode ="";
    MessageWindow *pMsgWin =new MessageWindow(nullptr);
    pMsgWin->setWindowState(charger::Platform::instance().defaultWinState);
    pMsgWin->setWindowTitle(message);
    pMsgWin->enableCloseButton();
    pMsgWin->show();
}


void PasswordUpdater::onPasswordEntered(QString userInput)
{
    switch (state)
    {
    case State::RequestCurrentPassword:
        // Check against current password
        if (userInput.toStdString().compare(passwordMgr.GetPassword()) == 0) {
            // Ask for new Password
            state = State::RequestNewPassword;
            passwordInputWin->RequestClear();
            passwordInputWin->SetPrompt(PassWordWin::Prompt::EnterNewPassword);
        }
        else {
            // Ask for password again
            passwordInputWin->RequestClear();
            showMessage(displayMsgs[DisplayMsg::CurrentPasswordError]);
        }
        break;
    case State::RequestNewPassword:
        // Save this new-password and Request to confirm
        state = State::RequestConfirmNewPassword;
        newInputPassword = userInput;
        passwordInputWin->RequestClear();
        passwordInputWin->SetPrompt(PassWordWin::Prompt::ConfirmNewPassword);
        break;
    case State::RequestConfirmNewPassword:
        // Check if the 2 passwords matches
        if (newInputPassword.compare(userInput) == 0) {
            // Save to file if match
            passwordMgr.SetPassword(newInputPassword.toStdString());
            // Display confirmation message
            showMessage(displayMsgs[DisplayMsg::PasswordUpdated]);
            // Done
            delete this;
        }
        else {
            // user to input again
            passwordInputWin->RequestClear();
            showMessage(displayMsgs[DisplayMsg::NewPasswordNotMatch]);
        }
        break;
    case State::Idle:
    default:
        throw "PasswordUpdater: unexpected state";
    }
}

void PasswordUpdater::onPasswordCanceled()
{
    // clean up
    delete this;
}
