#ifndef RESERVEWINDOW_H
#define RESERVEWINDOW_H

#include <QWidget>
#include <QTimer>
#include "chargerrequestrfid.h"

namespace Ui {
class reservewindow;
}

class reservewindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit reservewindow(QWidget *parent = nullptr);
    ~reservewindow();

private:
    Ui::reservewindow *ui;
    // CRfidThread    RFIDThread;
    std::unique_ptr<ChargerRequestRfid> rfidAuthorizer;
    bool                rfidAuthStarted;

    QTimer *m_Timer =NULL;
    u8     m_Delay =0;
    u8     msg_Delay = 0;
    u8     m_StayTimer =0;  //防止在刷卡自动充电时, 卡没及时取开而触发了停止充电

    QString        CardNo;
    QString        msgStyle;


private slots:
    // void on_RFIDCard(QByteArray cardno);
    void on_TimerEvent();

    void onRequestAccepted();
    void onRequestCanceled();
    void startRfidAuthorizor();
    void onChargeCommandAccepted();
};

#endif // RESERVEWINDOW_H
