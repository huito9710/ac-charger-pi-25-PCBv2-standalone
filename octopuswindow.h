#ifndef OCTOPUSWINDOW_H
#define OCTOPUSWINDOW_H

#include <QMainWindow>

namespace Ui {
class OctopusWindow;
}

class OctopusWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit OctopusWindow(QWidget *parent = 0);
    ~OctopusWindow();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::OctopusWindow *ui;
};

#endif // OCTOPUSWINDOW_H
