#include "octopuswindow.h"
#include "ui_octopuswindow.h"
#include "paymentmethodwindow.h"
#include "api_function.h"
#include "ccharger.h"
#include "chargerconfig.h"
#include "gvar.h"

OctopusWindow::OctopusWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::OctopusWindow)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setStyleSheet(QString("background-image: url(/home/pi/Desktop/BK/BG-P.5.%1.jpg);")
                        .arg(charger::getCurrDisplayLang() == charger::DisplayLang::English? "en" : "cn"));

    QString chargerNo("#");
    GetSetting(QStringLiteral("[CHARGER NO]"), chargerNo);
    ui->label_No->setText(chargerNo);
}

OctopusWindow::~OctopusWindow()
{
    delete ui;
}

void OctopusWindow::on_pushButton_clicked()
{ //HOME:
    m_ChargerA.UnLock();
    this->close();
    pCurWin =NULL;
}

void OctopusWindow::on_pushButton_3_clicked()
{
    pCurWin =new PaymentMethodWindow(this->parentWidget());
    pCurWin->setWindowState(Qt::WindowFullScreen);
    pCurWin->show();

    this->close();
}
