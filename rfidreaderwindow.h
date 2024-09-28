#ifndef RFIDREADERWINDOW_H
#define RFIDREADERWINDOW_H

#include <QMainWindow>
#include "chargerconfig.h"
#include "crfidthread.h"
#include "connstatusindicator.h"

namespace Ui {
class RFIDWindow;
}

class RfidReaderWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit RfidReaderWindow(QWidget *parent = nullptr);
    ~RfidReaderWindow();

    enum class Purpose {
        StartCharge,
        StopCharge,
        SuspendCharge,
    };

    enum class Message {
        Nil,
        AuthStarted,
        StartRejectedTryAgain,
        StopRejectedTryAgain,
        ServerOffline,
        StopTxnCardNotMatch
    };

    enum class RejectReason {
        Nil,
        Expired,
        TempBlocked,
        BlackListed,
    };

    void askForCard(Purpose);
    void displayMessage(Message msg, bool autoClose=true);
    void displayMessage(Message msg, RejectReason rejReason, bool autoClose=true);
    void requestClose();

    // Reader Status: received, user-cancel, error
    enum class ReaderStatus {
        CardReadOK,
        UserCancel,
        CardError,
    };
signals:
    void readerRespond(ReaderStatus newStatus, const QByteArray &cardNum = QByteArray());

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void on_pushButton_exit_clicked();
    void closeTemporaryMessage();
    void onRFIDCard(const QByteArray);

private:
    enum class State {
        PresentCard,
        CardCaptured,
        RetryMessage,
        PresentCardAgain,
    };
    void updateTitleLabel(Message msg);

private:
    Ui::RFIDWindow *ui;
    QString winBgStyle;
    QString succLabelStyle;
    QString failLabelStyle;
    Purpose purpose;
    State st;
    CRfidThread    readerThrd;
    ConnStatusIndicator connIcon;

    constexpr static int AutoCloseMessagePeriod = 5000; // 5-seconds
    constexpr static int ReducedAutoCloseMessagePeriod = 1000; // 1-seconds
    constexpr static int ReaderSerialPortRdTimeout = 500; // 0.5 sec
};

#endif // RFIDREADERWINDOW_H
