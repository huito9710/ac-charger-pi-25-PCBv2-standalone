#ifndef SETCHARGINGTIMEWIN_H
#define SETCHARGINGTIMEWIN_H

#include <QMainWindow>
#include <QTimer>
#include "chargerconfig.h"
#include "connstatusindicator.h"

namespace Ui {
class SetChargingTimeWin;
}

class SetChargingTimeWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit SetChargingTimeWin(QWidget *parent = 0);
    ~SetChargingTimeWin();

signals:
    void chargeTimeSet(bool completed);

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    void updateStyleStrs(charger::DisplayLang lang);

private slots:
    void on_pushButton_add_clicked();

    void on_pushButton_dec_clicked();

    void on_pushButton_ok_clicked();

    void on_pushButton_exit_clicked();

private:
    Ui::SetChargingTimeWin *ui;
    ConnStatusIndicator connIcon;
    ulong Second;   //设置值
    ulong dSecond;  //增量步长
    QString winBgStyle;
    bool reqCentralServerBeforeCharge;
    int timeSelectIndex;
    ulong *timeSelection = charger::GetTimeSelection();

};

#endif // SETCHARGINGTIMEWIN_H
