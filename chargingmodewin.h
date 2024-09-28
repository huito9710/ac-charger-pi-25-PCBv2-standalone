#ifndef CHARGINGMODEWIN_H
#define CHARGINGMODEWIN_H

#include <QMainWindow>
#include <QTimer>
#include <setchargingtimewin.h>

namespace Ui {
class ChargingModeWin;
}

class ChargingModeWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChargingModeWin(QWidget *parent = 0);
    ~ChargingModeWin();

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void on_pushButton_auto_clicked();

    void on_pushButton_time_clicked();

    void on_pushButton_delay_clicked();

    void on_pushButton_exit_clicked();

    void on_TimerEvent();

    void onChargeTimeSet(bool completed);

private:
    Ui::ChargingModeWin *ui;
    QString winBgStyle;
    QTimer connMonitorTimer;
    SetChargingTimeWin *chargeTimeWin;
};

#endif // CHARGINGMODEWIN_H
