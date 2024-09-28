#ifndef CABLEWIN_H
#define CABLEWIN_H

#include <QMainWindow>
#include <QTimer>
#include "chargerconfig.h"
#include "connstatusindicator.h"

namespace Ui {
class CableWin;
}

class CableWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit CableWin(QWidget *parent = 0);
    ~CableWin();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void on_pushButton_clicked();

    void on_pushButton_4_clicked();

    void on_TimerEvent();

    void on_passwordChangeButton_clicked();

    void on_tutorialButton_clicked();

    void on_langChangeButton_clicked();

private:
    void UpdateMessage();
    void initStyleStrings(charger::DisplayLang lang);

private:
    Ui::CableWin *ui;
    ConnStatusIndicator connIcon;
    QTimer  connMonitorTimer;
    int     lockDelay;
    int     idleTimeout;  //窗口在无Cable状态下超时计数, 以关闭窗口返回待机界面
    QString cableWindowStyle;
    QString socketTypeWindowStyle;
    QString tutorial0labelStyle;
    QString tutorial1labelStyle;
    const QString evConnLabelStyle;
    const QString evDisconnLabelStyle;
};

#endif // CABLEWIN_H
