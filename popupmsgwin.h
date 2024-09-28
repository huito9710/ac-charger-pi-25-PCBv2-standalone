#ifndef POPUPMSGWIN_H
#define POPUPMSGWIN_H

#include <QMainWindow>
#include "connstatusindicator.h"

namespace Ui {
class popupMsgWin;
}

class popupMsgWin : public QMainWindow
{
    Q_OBJECT

protected:
    void keyPressEvent(QKeyEvent *event) override;

public:
    explicit popupMsgWin(QWidget *parent = nullptr);
    ~popupMsgWin();
    void updateContent();

private slots:
    void on_quitButton_clicked();

private:
    Ui::popupMsgWin *ui;
    QString winBgStyle;
    ConnStatusIndicator connIcon;
};

#endif // POPUPMSGWIN_H
