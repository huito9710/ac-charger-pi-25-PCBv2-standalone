#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include <QMainWindow>
#include "connstatusindicator.h"

namespace Ui {
class MessageWindow;
}

class MessageWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MessageWindow(QWidget *parent = 0);
    ~MessageWindow();

protected:
    void showEvent(QShowEvent * event);

public:
    void UpdateMessage();
    void enableCloseButton();

private slots:
    void on_pushButton_2_clicked();

private:
    Ui::MessageWindow *ui;
    ConnStatusIndicator connIcon;
};

#endif // MESSAGEWINDOW_H
