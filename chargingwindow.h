#ifndef CHARGINGWINDOW_H
#define CHARGINGWINDOW_H

#include <QMainWindow>
#include <QTimer>
#if false
#include <QTableWidgetItem>
#else
#include <QGridLayout>
#include <QLabel>
#endif
#include "chargerconfig.h"
#include "chargerrequestrfid.h"
#include "connstatusindicator.h"
#include "suspendchargingwin.h"

namespace Ui {
class ChargingWindow;
}

class ChargingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChargingWindow(QWidget *parent = 0);
    ~ChargingWindow();

    unsigned int getObjectNum();
    static unsigned int getInstanceCount();
    static ChargingWindow * getInstanceFromPool(int at);

protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void on_quitButton_clicked();
    void on_pauseButton_clicked();
    void onChargeMonitorTimerEvent();
    void onRfidAuthAccepted();
    void onRfidAuthCanceled();
    void onChargerStopChargeRequested();
    void onStandaloneModeAuthWinDestroyed(QObject *);

    void on_unlockButton_clicked();
    void onUnlockTimerEvent();

    void onResumeFromSuspension(int res);

private:
    void updateStyleStrings(enum charger::DisplayLang lang, enum charger::ChargeMode chargeMode);
    void buildDetailsGridUI(enum charger::ChargeMode chargeMode, int phases);
    void setGridLabel(QLabel &label, QString text, Qt::Alignment align);
    QString makeDisplayTextOfPhaseValues(const double val, const int phaseNum, const int decPlaces, const int numPhases);
    void updateTimeLabel(QLabel &hhLabel, QLabel &mmLabel, unsigned long timeVal);
    void updateButtonStates();
    void prepareStandaloneModeAuthWin();
    bool debounceMinCurrent(); // return true if debounced and assert
    void unlockConnectorAndSetTimer();

    void quitOnResumeFromSuspension();
    void pauseButton_execution();
private:
    Ui::ChargingWindow *ui;
    QMainWindow *standaloneModeAuthWin;
    std::unique_ptr<ChargerRequestRfid> rfidAuthorizer;

    QTimer chargeMonitorTimer;
    int    m_SaveTime;
    bool   stopChargingRequested;
    bool   waitRfidReleaseConn;
    static constexpr int debouncePeriod  = 5;
    int    minCurrentDebounceCnt;
    QTimer unlockTimer;
    const std::chrono::seconds unlockTimeoutPeriod;
    bool   prompting4Suspend = false;

    QString winBgStyle;
    QString exitButtonStyle;
    QString exitLabelStyle;
    QString safeModeLabelStyle;
    QString pauseButtonStyle;
    QString pauseLabelStyle;

    QString ocppTransId;

    // Grid widgets
    QGridLayout chargeDetailsGrid;

    QLabel bookedTimeRhLabel;
    QLabel remainTimeRhLabel;
    QLabel voltageRhLabel;
    QLabel currentRhLabel;
    QLabel energyRhLabel;

    QLabel chargeHourUnitLabel;
    QLabel chargeMinuteUnitLabel;
    QLabel remainHourUnitLabel;
    QLabel remainMinuteUnitLabel;
    QLabel voltageUnitLabel;
    QLabel currentUnitLabel;
    QLabel voltage1UnitLabel;
    QLabel energyUnitLabel;

    QLabel chargeHourValLabel;
    QLabel remainHourValLabel;
    QLabel chargeMinuteValLabel;
    QLabel remainMinuteValLabel;
    QLabel voltageValLabel[3];
    QLabel currentValLabel[3];
    QLabel energyValLabel;

    QLabel loadmanagemsg;
    QString loadmanageStyle;

    ConnStatusIndicator connIcon;

    SuspendChargingWindow * suspendWindow;
    unsigned int objectNum;
    static unsigned int instanceCount;
    static QList<ChargingWindow *> objectPool;
};

#endif // CHARGINGWINDOW_H
