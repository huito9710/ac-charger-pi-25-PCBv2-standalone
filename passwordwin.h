#ifndef PASSWORDWIN_H
#define PASSWORDWIN_H

#include <QMainWindow>
#include <QTimer>
#include "chargerconfig.h"


namespace Ui {
class PassWordWin;
}

class PassWordWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit PassWordWin(QWidget *parent = 0);
    ~PassWordWin();

    enum class Prompt {
        Default,
        EnterCurrentPassword,
        EnterNewPassword,
        ConfirmNewPassword
    };

    void RequestClose();
    void RequestClear();
    void SetPrompt(Prompt);

private:
    void On_numberKey(char key);
    void initStyleStrings(charger::DisplayLang lang);

signals:
    void passwordEntered(QString password);
    void quitRequested();

private slots:
    void on_pushButton_0_clicked();

    void on_pushButton_1_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_9_clicked();

    void on_pushButton_clr_clicked();

    void on_pushButton_ok_clicked();

    void on_pushButton_exit_clicked();

private:
    Ui::PassWordWin *ui;
    Prompt prompt;
    QString m_passWord;
    QString m_WindowBackgroundStyle;
    const QString m_DigitInputStyle;
    const QString m_DigitClearedStyle;
};

#endif // PASSWORDWIN_H
