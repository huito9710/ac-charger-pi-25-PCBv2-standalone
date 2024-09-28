#ifndef CHARGINGMODEIMMEDIATEWIN_H
#define CHARGINGMODEIMMEDIATEWIN_H

#include <QMainWindow>
#include <QObject>
#include <QTimer>
#include <setchargingtimewin.h>
#include "connstatusindicator.h"

namespace Ui {
class ChargingModeWin;
}

class ChargingModeImmediateWin : public QMainWindow
{
    Q_OBJECT
public:
    explicit ChargingModeImmediateWin(QWidget *parent = nullptr);
    ~ChargingModeImmediateWin();

signals:

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void on_pushButton_exit_clicked();

    void on_pushButton_auto_clicked();

    void on_pushButton_time_clicked();

    void onCableMonitorTimeout();

    void onChargeTimeSet(bool completed);

private:
    Ui::ChargingModeWin *ui;
    QString winBgStyle;
    QTimer cableMonitorTimer;
    SetChargingTimeWin *chargeTimeWin;
    ConnStatusIndicator connIcon;

    static constexpr int cableMonitorInterval = 500; //half-second
};

#endif // CHARGINGMODEIMMEDIATEWIN_H
