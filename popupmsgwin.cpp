#include "popupmsgwin.h"
#include "ui_popupmsgwin.h"
#include "hostplatform.h"
#include "gvar.h"
#include "statusledctrl.h"

using namespace charger;

popupMsgWin::popupMsgWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::popupMsgWin),
    winBgStyle(QString("QWidget#centralwidget { background-image: url(%1emptyMsgWin.jpg);}")
             .arg(Platform::instance().guiImageFileBaseDir)),
    connIcon(m_OcppClient)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->connStatusIcon->setHidden(charger::isStandaloneMode());
    connIcon.setIndicator(ui->connStatusIcon);
    this->setStyleSheet(winBgStyle);

    QString quitButtonStyle = QString("QPushButton{background:transparent; border-image:url(%1PNG/quit.%2.png); image:none;} QPushButton:checked{image: url(%1PNG/Rect2.png);}")
            .arg(Platform::instance().guiImageFileBaseDir)
            .arg(charger::getCurrDisplayLang() == charger::DisplayLang::Chinese? "ZH" : "EN");
    if(m_ChargerA.popupIsQueueingMode == 1)
    {
        ui->quitButton->setStyleSheet(quitButtonStyle);
        if(charger::isHardKeySupportRequired())
        {
            ui->quitButton->setCheckable(true);
            ui->quitButton->setChecked(true);
        }
    }
    else
    {
        ui->quitButton->setCheckable(false);
        ui->quitButton->setChecked(false);
    }

    QString TitleStyle = QString("color:#%1; font-family: Piboto, Calibri, sans-serif; font-size: 28px")
            .arg(m_ChargerA.popupMsgTitleColor);
    QString ContentStyle = QString("color:#%1; font-family: Piboto, Calibri, sans-serif; font-size: %2px")
            .arg(m_ChargerA.popupMsgContentColor)
            .arg(m_ChargerA.popupMsgContentFontSize);


    QString titleText;
    QString contentText;
    if(charger::getCurrDisplayLang() == charger::DisplayLang::Chinese)
    {
        titleText = m_ChargerA.popupMsgTitlezh;
        contentText = m_ChargerA.popupMsgContentzh;
    }
    else
    {
        titleText = m_ChargerA.popupMsgTitleen;
        contentText = m_ChargerA.popupMsgContenten;
    }
    ui->titleMsg->setStyleSheet(TitleStyle);
    ui->titleMsg->setText(titleText);
    ui->contentMsg->setStyleSheet(ContentStyle);
    ui->contentMsg->setText(contentText);

}

popupMsgWin::~popupMsgWin()
{
    delete ui;
}

void popupMsgWin::on_quitButton_clicked()
{
    if(m_ChargerA.popupIsQueueingMode == 1 && m_ChargerA.IsCharging())
    {
        m_ChargerA.StopCharge();
    }
    emit m_OcppClient.cancelPopUpMsgReceived();
//    this->close();
//    pCurWin->show();
}

void popupMsgWin::updateContent()
{
    QString titleText;
    QString contentText;
    if(charger::getCurrDisplayLang() == charger::DisplayLang::Chinese)
    {
        titleText = m_ChargerA.popupMsgTitlezh;
        contentText = m_ChargerA.popupMsgContentzh;
    }
    else
    {
        titleText = m_ChargerA.popupMsgTitleen;
        contentText = m_ChargerA.popupMsgContenten;
    }
    ui->titleMsg->setText(titleText);
    ui->contentMsg->setText(contentText);
}

void popupMsgWin::keyPressEvent(QKeyEvent *event)
{
    qWarning() << "popupMsgWin: Key press event";
    if(event->key() ==Qt::Key_Return)
    {
        qDebug() <<"key exit";
        if(ui->quitButton->isEnabled()) on_quitButton_clicked();
    }
}
