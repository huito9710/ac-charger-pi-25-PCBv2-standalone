#ifndef SUSPENDCHARGINGWIN_H
#define SUSPENDCHARGINGWIN_H

#include <QMainWindow>
#include <QTimer>
#include "chargerconfig.h"
#include "connstatusindicator.h"

namespace Ui {
class SuspendChargingWindow;
}

class SuspendChargingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SuspendChargingWindow(QWidget *parent = 0);
    ~SuspendChargingWindow();
    QString transId;

    static int instanceCount;

signals:
    void finishSuspension(int res);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updateStyleStrs(charger::DisplayLang lang);

private slots:
    void on_pushButton_resume_clicked();
    void on_pushButton_exit_clicked();
    void on_cableConn_timeout();
    void updateTime();

private:
    Ui::SuspendChargingWindow *ui;
    QTimer * countDownTimer;
    QTimer * cableConnTimer;
    int m_countDown;

    ConnStatusIndicator connIcon;
    bool reqCentralServerBeforeCharge;
};


#endif // SUSPENDCHARGINGWIN_H
