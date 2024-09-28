#include "wapwindow.h"
#include "ui_wapwindow.h"
#include "paymentmethodwindow.h"
#include "ccharger.h"
#include "gvar.h"

WapWindow::WapWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WapWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    if(charger::getCurrDisplayLang() == charger::DisplayLang::English)
    {
        this->setStyleSheet("background-image: url(/home/pi/Desktop/BK/BG-P.6.en.jpg);");
    }
    else
    {
        this->setStyleSheet("background-image: url(/home/pi/Desktop/BK/BG-P.6.cn.jpg);");
    }
}

WapWindow::~WapWindow()
{
    delete ui;
}

void WapWindow::on_pushButton_clicked()
{ //HOME:
    m_ChargerA.UnLock();
    this->close();
    pCurWin =NULL;
}

void WapWindow::on_pushButton_3_clicked()
{
    pCurWin =new PaymentMethodWindow(this->parentWidget());
    pCurWin->setWindowState(Qt::WindowFullScreen);
    pCurWin->show();

    this->close();
}
