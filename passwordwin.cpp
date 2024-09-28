#include "passwordwin.h"
#include "ui_passwordwin.h"
#include "api_function.h"
#include "gvar.h"
#include "chargingmodewin.h"
#include "waittingwindow.h"
#include "hostplatform.h"
#include "passwordmgr.h"
#include "chargerconfig.h"

PassWordWin::PassWordWin(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PassWordWin),
    prompt(Prompt::Default),
    m_DigitInputStyle(QString("background:transparent; border-image: url(%1PNG/pwd.png);").arg(charger::Platform::instance().guiImageFileBaseDir)),
    m_DigitClearedStyle("background:transparent; border-image:transparent;")
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    initStyleStrings(charger::getCurrDisplayLang());
    this->setStyleSheet(m_WindowBackgroundStyle);

    m_passWord ="";
}

PassWordWin::~PassWordWin()
{
    delete ui;
}

void PassWordWin::initStyleStrings(charger::DisplayLang lang)
{
    using namespace charger;
    QString langInFn;
    switch (lang)
    {
    case DisplayLang::English:
        langInFn = "EN";
        break;
    case DisplayLang::Chinese:
    case DisplayLang::SimplifiedChinese:
        langInFn = "ZH";
        break;
    }

    QString fileBaseName;
    switch (prompt) {
    case Prompt::Default: fileBaseName = "passwordWin"; break;
    case Prompt::EnterCurrentPassword: fileBaseName = "EnterCurrentPassword"; break;
    case Prompt::EnterNewPassword: fileBaseName = "EnterNewPassword"; break;
    case Prompt::ConfirmNewPassword: fileBaseName = "ConfirmNewPassword"; break;
    }
    m_WindowBackgroundStyle = QString("background-image: url(%1%2.%3.jpg);").arg(charger::Platform::instance().guiImageFileBaseDir)
            .arg(fileBaseName).arg(langInFn);
}

void PassWordWin::on_pushButton_0_clicked()
{
    On_numberKey('0');
}

void PassWordWin::on_pushButton_1_clicked()
{
    On_numberKey('1');
}

void PassWordWin::on_pushButton_2_clicked()
{
    On_numberKey('2');
}

void PassWordWin::on_pushButton_3_clicked()
{
    On_numberKey('3');
}

void PassWordWin::on_pushButton_4_clicked()
{
    On_numberKey('4');
}

void PassWordWin::on_pushButton_5_clicked()
{
    On_numberKey('5');
}

void PassWordWin::on_pushButton_6_clicked()
{
    On_numberKey('6');
}

void PassWordWin::on_pushButton_7_clicked()
{
    On_numberKey('7');
}

void PassWordWin::on_pushButton_8_clicked()
{
    On_numberKey('8');
}

void PassWordWin::on_pushButton_9_clicked()
{
    On_numberKey('9');
}

void PassWordWin::on_pushButton_clr_clicked()
{
    m_passWord ="";
    ui->label_1->setStyleSheet(m_DigitClearedStyle);
    ui->label_2->setStyleSheet(m_DigitClearedStyle);
    ui->label_3->setStyleSheet(m_DigitClearedStyle);
    ui->label_4->setStyleSheet(m_DigitClearedStyle);
}

void PassWordWin::on_pushButton_ok_clicked()
{
    emit passwordEntered(m_passWord);
}

void PassWordWin::on_pushButton_exit_clicked()
{
    emit quitRequested();
}

void PassWordWin::On_numberKey(char key)
{
    int len =m_passWord.length();

    if(len <4)
    {
        m_passWord.append(key);

        len++;
        switch (len) {
        case 1:
            ui->label_1->setStyleSheet(m_DigitInputStyle); break;
        case 2:
            ui->label_2->setStyleSheet(m_DigitInputStyle); break;
        case 3:
            ui->label_3->setStyleSheet(m_DigitInputStyle); break;
        case 4:
            ui->label_4->setStyleSheet(m_DigitInputStyle); break;
        }
    }
}

void PassWordWin::RequestClose()
{
    if(pCurWin ==this)  //开启充电时才会有设置pCurWin=本窗口
    {
        pCurWin =NULL;
    }
    this->close();
}

void PassWordWin::RequestClear()
{
    on_pushButton_clr_clicked();
}

void PassWordWin::SetPrompt(Prompt newPrompt)
{
    using namespace charger;
    prompt = newPrompt;
    initStyleStrings(charger::getCurrDisplayLang());
    this->setStyleSheet(m_WindowBackgroundStyle);
}
