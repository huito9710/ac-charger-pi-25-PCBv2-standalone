#ifndef PASSWORDUPDATER_H
#define PASSWORDUPDATER_H
#include <map>
#include <QObject>
#include "passwordmgr.h"
#include "passwordwin.h"

class PasswordUpdater : public QObject
{
    Q_OBJECT
public:
    explicit PasswordUpdater(QObject *parent = nullptr);
    ~PasswordUpdater();
    void start();

signals:
    void updated();
    void canceled();

private:
    enum class State {
        Idle,
        RequestCurrentPassword,
        RequestNewPassword,
        RequestConfirmNewPassword,
    };
    enum class DisplayMsg {
        CurrentPasswordError,
        NewPasswordNotMatch,
        PasswordUpdated,
    };

private slots:
    void onPasswordEntered(QString userInput);
    void onPasswordCanceled();

private:
    void showMessage(QString message);
    void showPasswordInputWin(PassWordWin::Prompt prompt);

private:
    State state;
    std::map<DisplayMsg, QString> displayMsgs;
    PasswordMgr passwordMgr;
    PassWordWin *passwordInputWin;
    QString newInputPassword;
};

#endif // PASSWORDUPDATER_H
