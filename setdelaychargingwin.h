#ifndef SETDELAYCHARGINGWIN_H
#define SETDELAYCHARGINGWIN_H

#include <QMainWindow>
#include <QTimer>

namespace Ui {
class SetDelayChargingWin;
}

class SetDelayChargingWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit SetDelayChargingWin(QWidget *parent = 0);
    ~SetDelayChargingWin();

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void on_pushButton_add_clicked();

    void on_pushButton_dec_clicked();

    void on_pushButton_ok_clicked();

    void on_pushButton_exit_clicked();

    void on_TimerEvent();

private:
    Ui::SetDelayChargingWin *ui;

    QTimer connMonitorTimer;
    ulong Second;   //设置值
    ulong dSecond;  //增量步长
    QString winBgStyle;
};

#endif // SETDELAYCHARGINGWIN_H
