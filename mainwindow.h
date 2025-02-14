#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ccomportthreadv2.h"
#include "distancesensorthread.h"
#include "T_Type.h"
#include "chargerconfig.h"
#include "OCPP/ocppclient.h"
#include "firmwareupdatescheduler.h"
#include "evchargingtransactionregistry.h"
#include "OCPP/ocpptransactionmessagerequester.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
//void mousePressEvent(QMouseEvent * event);
    void changeEvent(QEvent *event) override;

private slots:
    void Blink(void);       //0.5" Timer
    void SearchCard(void);
    void ExtKeyDown(const char keycode);
    void on_pushButton_clicked();

    void onOcppConnected();
    void onCheckTransactionTimeout();
    void onOcppConnectFailed();
    void onOcppDisconnected();
    void onOcppCallTimedout();
    void onOcppConnRetryWaited();
    void onOcppBootNotificationAccepted();
    void onServerDateTimeReceived(const QDateTime currTime);

    void onFirmwareDownloadStarted();
    void onFirmwareDownloadCompleted(const QString &localPath);
    void onFirmwareDownloadFailed(const QString &errStr);
    void onFirmwareInstallReady(const QString &localPath);
    void onFirmwareInstallFailed(const QString &errStr);
    void onFirmwareInstallCompleted();

    void onConnAvailabilityChanged(bool newVal);
    void onScheduleConnUnavailChange() { connUnAvailPending = true; }; // Always change to Inoperative
    void onCancelScheduleConnUnavailChange() { connUnAvailPending = false; }; // Always change to Inoperative

    void onChargeCommandAccepted(QDateTime startDt, unsigned long startMeterVal, bool isResumedCharging, int resumedTransId);
    void onChargeCommandCanceled(QDateTime canceledDt, unsigned long stopMeterVal, ocpp::Reason cancelReason);
    void onChargeFinished(QDateTime stopDt, unsigned long stopMeterVal, ocpp::Reason stopReason);
    void onChargerErrorCodeChanged();

    void onMeterSampleIntervalCfgChanged();

    void onTransactionCompleted(const QString transactionId); // Start, MeterValue(s), Stop
    void onTransactionError(const QString idTag, const QString transactionId);

    void onPwrMeterRecSaveTimer();

    void StopResumedChargingTransaction(const QString transId, const QDateTime stopDt, const unsigned long stopMeterVal, const ocpp::Reason reason, const QString tagId);
    void oncheckUnlockTimerEvent();
    void checkunlock();
    void hardreset();

    void onParkingStateChanged(QString pState,float dist);
    
    void onPopUpMsgReceived();
    void onCancelPopUpMsgReceived();

    void onChangeLedLight(int color);


private:
    Ui::MainWindow *ui;
    QString winByStyle;
    QSerialPort stm32SerialPort;
    CComPortThreadV2 ChargerThreadA;
    DistanceSensorThread DistSensorThread;
    QTimer ocppConnRetryTimer;
    const std::chrono::seconds ocppConnRetryWaitInterval;
    bool rebootPending;
    const QString networkOfflineMsgWinTitle;
    const QString connUnavailMsgWinTitle;
    const QString stationBusyMsgWinTitle;
    bool connUnAvailPending;
    OcppTransactionMessageRequester ocppTransRequester;
    const std::chrono::minutes pwrMeterRecSaveInterval;
    QTimer pwrMeterRecSaveTimer;
    bool offlinemessagelock;

    QTimer checkunlockTimer;
    const std::chrono::seconds checkunlockTimeoutPeriod;

    QTimer closePopUpMsgTimer;
    const QString popUpMsgWinTitle;

private:
    void updateStyleStrs(bool wallSocketSupported, charger::DisplayLang lang);
    void on_FoundCard(void);
    void ChargerControl();
    void displayOfflineMessage();
    void closeOfflineMessage();
    void displayConnUnavailMessage();
    void closeConnUnavailMessage();
    void displayStationBusyMessage();
    void closeStationBusyMessage();
};

#endif // MAINWINDOW_H

