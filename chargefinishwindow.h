#ifndef CHARGEFINISHWINDOW_H
#define CHARGEFINISHWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "paymentmethodwindow.h"
#include "chargerconfig.h"
#include "chargerrequestrfid.h"
#include "connstatusindicator.h"

namespace Ui {
class ChargeFinishWindow;
}

class ChargeFinishWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChargeFinishWindow(QString transId = QString(), QWidget *parent = 0);
    ~ChargeFinishWindow();

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void on_pushButton_clicked();
    void on_TimerEvent();
    void onCheckOcppResponseTimeout();
    void checkStartPaymentWindow();
    void checkStopPaymentWindow();
    void onBackgroundPaymentWindowClosed() { this->close(); }

private:
    enum class ChargeSummary {
        byTime,
        byTimeWithConnFailure,
        byEnergy,
    };
private:
    void updateStyleStrs(ChargeSummary chrSummary, charger::DisplayLang lang);
    void setQuitButtonEnable(bool enableIt);

private:
    const std::chrono::milliseconds cableCheckTimerInterval;
    const std::chrono::milliseconds defaultMinDisplayTime;
    const std::chrono::milliseconds txnCheckTimerInterval;
    Ui::ChargeFinishWindow *ui;
    ConnStatusIndicator connIcon;
    QString winBgStyle;
    QTimer cableCheckTimer;
    QTimer txnCheckTimer;
    std::chrono::milliseconds displayedTime;
    QString ocppTransId;
    bool ocppCsNotResponding;
    bool txnPendingToCs;
    bool paymentWinRunning; // Capture RFID card to start charging again.
    PaymentMethodWindow *paymentWin;
};

#endif // CHARGEFINISHWINDOW_H
