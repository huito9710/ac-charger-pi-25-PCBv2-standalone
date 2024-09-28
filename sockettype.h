#ifndef SOCKETTYPE_H
#define SOCKETTYPE_H

#include <QMainWindow>
#include <QTimer>
#include "chargerconfig.h"
#include "connstatusindicator.h"

namespace Ui {
class SocketType;
}

class SocketType : public QMainWindow
{
    Q_OBJECT

public:
    explicit SocketType(QWidget *parent = 0);
    ~SocketType();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

protected slots:

private slots:

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_TimerEvent();

    void on_passwordChangeButton_clicked();

    void on_langChangeButton_clicked();

private:
    void initStyleStrings(charger::DisplayLang lang);

private:
    Ui::SocketType *ui;
    ConnStatusIndicator connIcon;
    QTimer cableMonitorTimer;
    QString socketTypeWinBgStyle;
    const QString connSelCheckedButtonStyle;
    const QString langChangeCheckedButtonStyle;
    const QString settingsCheckedButtonStyle;
};

#endif // SOCKETTYPE_H
