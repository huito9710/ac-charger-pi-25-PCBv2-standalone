#ifndef WAPWINDOW_H
#define WAPWINDOW_H

#include <QMainWindow>

namespace Ui {
class WapWindow;
}

class WapWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit WapWindow(QWidget *parent = 0);
    ~WapWindow();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::WapWindow *ui;
};

#endif // WAPWINDOW_H
