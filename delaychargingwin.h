#ifndef DELAYCHARGINGWIN_H
#define DELAYCHARGINGWIN_H

#include <QMainWindow>
#include <QTimer>

namespace Ui {
class DelayChargingWin;
}

class DelayChargingWin : public QMainWindow
{
    Q_OBJECT

public:
    explicit DelayChargingWin(QWidget *parent = 0);
    ~DelayChargingWin();

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void on_pushButton_exit_clicked();

    void on_TimerEvent();

private:
    Ui::DelayChargingWin *ui;

    QTimer *m_Timer =NULL;
    QString winBgStyle;
};

#endif // DELAYCHARGINGWIN_H
