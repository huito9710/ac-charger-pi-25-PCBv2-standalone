#ifndef PAYMENTMETHODWINDOW_H
#define PAYMENTMETHODWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "chargerrequestrfid.h"
#include "chargerconfig.h"
#include <utility>
#include "connstatusindicator.h"

namespace Ui {
class PaymentMethodWindow;
}

class PaymentMethodWindow : public QMainWindow
{
    Q_OBJECT
public:
    static constexpr int qrCodeImageBorderWidth = 8;

public:
    explicit PaymentMethodWindow(QWidget *parent = 0);
    ~PaymentMethodWindow();

    void runInBackground();

protected:
    void showEvent(QShowEvent *);
    void keyPressEvent(QKeyEvent *event);

private:

    enum class OcppAuthStatus {
        Auth_Idle,
        Auth_Started,
        Auth_Rejected,
    };

    void initPayMethod();
    void updateStyleStrs(charger::PayMethod, charger::DisplayLang);
    //void displayChargerNum(charger::PayMethod payMethod, int chargerNum);
    void displayChargerNum(charger::PayMethod payMethod, QString chargerNumStr);
    void displaySerialNum(charger::PayMethod payMethod, QString serialNum);
    void displayChargerName(charger::PayMethod payMethod, QString chargerName);


signals:
    void bgWindowClosed();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_TimerEvent();
    void startRfidAuthorizor();
    void onRequestAccepted();
    void onRequestCanceled();

    void onChargeCommandAccepted();

private:

    Ui::PaymentMethodWindow *ui;
    QTimer              chargerConnCheckTimer;
    charger::PayMethod  payMethod;
    std::unique_ptr<ChargerRequestRfid> rfidAuthorizer;
    bool                rfidAuthStarted;
    QString             winBgStyle;
    bool                bgMode;
    ConnStatusIndicator connIcon;

    QString chgrNumLabelLgStyle;
    QString chgrNumLabelSmStyle;

};

#endif // PAYMENTMETHODWINDOW_H
