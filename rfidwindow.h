#ifndef RFIDWINDOW_H
#define RFIDWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "crfidthread.h"
#include "chargerconfig.h"
#include "connstatusindicator.h"

namespace Ui {
class RFIDWindow;
}

class RFIDWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit RFIDWindow(QWidget *parent = 0);
    ~RFIDWindow();

public:
    void WaitRemoveCard();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    enum class RfidReqStatus {
        PresentCard,
        Success,
        Failed,
    };
    enum class UserRequest {
        Authorize,
        StopCharge,
    };

private slots:
    void on_pushButton_exit_clicked();
    void on_TimerEvent();
    void on_RFIDCard(QByteArray cardno);

private:
    void updateWinBgStyle(RfidReqStatus rfidReqSt, UserRequest usrReq, charger::DisplayLang lang);
    void setWinBg(RfidReqStatus rfidReqSt, UserRequest usrReq);

private:
    Ui::RFIDWindow *ui;
    ConnStatusIndicator connIcon;
    QString     winBgStyleStr;
    const QString  noReaderImageFilePath;
    const QString  hwStatusLabelStyleStr;
    CRfidThread    RFIDThread;
    QString        CardNo;
    QTimer *m_Timer =nullptr;
    u8     m_Delay =0;
    u8     m_StayTimer =0;  //防止在刷卡自动充电时, 卡没及时取开而触发了停止充电
};

#endif // RFIDWINDOW_H
