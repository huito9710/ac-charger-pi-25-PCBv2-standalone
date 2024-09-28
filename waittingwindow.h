#ifndef WAITTINGWINDOW_H
#define WAITTINGWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "chargerconfig.h"
#include "chargerrequestrfid.h"
#include "connstatusindicator.h"

namespace Ui {
class WaittingWindow;
}

class WaittingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit WaittingWindow(QWidget *parent = 0);
    ~WaittingWindow();
    void setSeekApprovalBeforeCharge(bool doSeek) { reqCentralServerBeforeCharge = doSeek; };

protected:
    void keyPressEvent(QKeyEvent *event);
    void showEvent(QShowEvent * event);

private:
    enum class BgMessage {
        Connecting,
        RequestFailed,
    };

private slots:
    void on_pushButton_exit_clicked();

    void on_TimerEvent();
    void onRfidAuthAccepted();
    void onRfidAuthCanceled();
    void on_OcppResponse(QString datatransfer);

private:
    void updateWinBgStyleString(enum BgMessage msg, charger::DisplayLang lang);
    void showRequestFailedMsg();

private:
    Ui::WaittingWindow *ui;
    ConnStatusIndicator connIcon;
    std::unique_ptr<ChargerRequestRfid> rfidAuthorizer;
    QString winBgStyle;
    QTimer  cableMonitorTimer;
    bool reqCentralServerBeforeCharge;
    bool quitRequested;

    static constexpr int cableMonitorInterval = 500; //half-second
};

#endif // WAITTINGWINDOW_H
