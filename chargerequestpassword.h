#ifndef CHARGEREQUESTPASSWORD_H
#define CHARGEREQUESTPASSWORD_H

#include <QObject>
#include <QMainWindow>
#include "passwordwin.h"

class ChargeRequestPassword : public QObject
{
    Q_OBJECT
public:
    explicit ChargeRequestPassword(QObject *parent = nullptr);
    ~ChargeRequestPassword();
    QMainWindow *Start(QWidget *parent);

signals:
    void requestCompleted(bool success);

private:
    bool isPasswordValid(const QString &userInputCode);
    void quit(bool passwordAccepted);
    void closePasswordWin() {  if (passwordWin) {
            passwordWin->RequestClose();
            passwordWin = nullptr;
        } } ;

private slots:
    void onPasswordEntered(QString userInput);
    void onPasswordCanceled();
    void onTimerEvent();

private:
    PassWordWin *passwordWin;
    QTimer chargerConnCheckTimer;
    const int timerPeriod = 500; // milliseconds
};

#endif // CHARGEREQUESTPASSWORD_H
