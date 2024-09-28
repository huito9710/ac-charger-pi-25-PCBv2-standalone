#include "videoplay.h"
#include "ui_videoplay.h"
#include <QProcess>
#include <QtCore/QDebug>
#include "api_function.h"
#include <QDateTime>
#include <QtCore/QDebug>
#include <QFileInfo>
#include <QFile>
#include "gvar.h"
#include "hostplatform.h"

extern QMainWindow  *pVideoWin;
extern u16          m_IdleTime;
extern u8           m_VideoNo;  //0:屏保或广告视频  1:教学视频

const char VideoPlay::defaultTutorialVideo[] = "sample.mp4";

QString VideoPlay::tutorialsFile()
{
    QString videoFile(charger::Platform::instance().videosDir); // default location
    GetSetting(QStringLiteral("[videoFileDir]"), videoFile); // override with settings.ini (if any)
    QString tutorialFile(VideoPlay::defaultTutorialVideo); // default tutorial file
    GetSetting(QStringLiteral("[TutorialsFile]"), tutorialFile); // override with settings.ini (if any)
    videoFile += tutorialFile;

    return videoFile;
}

QString VideoPlay::featuredVideoFile()
{
    QString videoFile(charger::Platform::instance().videosDir); // default location
    GetSetting(QStringLiteral("[videoFileDir]"), videoFile); // override with settings.ini (if any)
    QString featuredVideo(VideoPlay::defaultTutorialVideo); // default featured video file
    GetSetting(QStringLiteral("[videoFile]"), featuredVideo); // override with settings.ini (if any)
    videoFile += featuredVideo;

    return videoFile;
}

bool VideoPlay::withinAllowedTime(const QTime &curr, const QTime &start, const QTime &end)
{
    // Not specified time-range implies Allowed
    if (start.isNull() || !start.isValid() || end.isNull() || !end.isValid())
        return true;
    bool allowed = false;
    // Case 1: EndTime is before-midnight
    //          StartTime < EndTime:  StartTime <  CurrentTime < EndTime
    // Case 2: EndTime is past-midnight
    //         EndTime < StartTime: CurrentTime > StartTime || CurrentTime < EndTime
    if (start < end) {
        allowed = (curr >= start) && (curr <= end);
    }
    else {
       allowed = (curr >= start) || (curr <= end);
    }
    return allowed;
}

bool VideoPlay::shouldDisableFeatureVideo()
{
    QString videoOnOffSetting ="";
    GetSetting(QStringLiteral("[VIDEO]"), videoOnOffSetting);

    QString startTimeSetting("00:00"), endTimeSetting("24:00"); // default values
    GetSetting(QStringLiteral("[VIDEO ENABLE TIME]"), startTimeSetting);
    GetSetting(QStringLiteral("[VIDEO DISABLE TIME]"), endTimeSetting);

    // Take care of case where the end time is on or past midnight;
    int colonPos = endTimeSetting.indexOf(':');
    if (colonPos > 0) {
        int hr = endTimeSetting.left(colonPos).toInt();
        if (hr > 23) {
            hr %= 24;
            // make it into 2 character string, append with the remaining time (:mm)
            endTimeSetting = QString("%1%2").arg(hr, 2, 10, QLatin1Char('0')).arg(endTimeSetting.mid(colonPos));
        }
    }

    QTime currTime = QTime::currentTime();
    QTime startTime = QTime::fromString(startTimeSetting, "HH:mm");
    QTime endTime = QTime::fromString(endTimeSetting, "HH:mm");

    // Case 1: EndTime is before-midnight
    //          StartTime < EndTime:  StartTime <  CurrentTime < EndTime
    // Case 2: EndTime is past-midnight
    //         EndTime < StartTime: CurrentTime > StartTime || CurrentTime < EndTime
    bool disableVideo = (videoOnOffSetting == "0")
            || (!withinAllowedTime(currTime, startTime, endTime));
    return disableVideo;
}

VideoPlay::VideoPlay(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VideoPlay)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);

    playform =new QWidget(this);
    playform->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred));
    playform->setAttribute(Qt::WA_OpaquePaintEvent);
    playform->move(0, 0);
    playform->setMinimumSize(800, 480);

    //{{
    QString txt = (m_VideoNo ==0)? featuredVideoFile() : tutorialsFile();

    if(m_VideoNo ==0)       //屏保或广告视频
    {
        if(shouldDisableFeatureVideo())
        {
            system("xset dpms force off \n");
            //qDebug() <<timetxt;
        }
    }

#ifdef VLC_AVAILABLE
    if(QFile::exists(txt) &&(QFileInfo(txt).isFile())) vlcInstance =libvlc_new(0, NULL);
    if(vlcInstance)
    {
        libvlc_media_t *vlcMedia =libvlc_media_new_path(vlcInstance, txt.toUtf8().constData());
        if(vlcMedia)
        {
            if(playform) vlcPlayer =libvlc_media_player_new_from_media(vlcMedia);
            //vlcPlayer =libvlc_media_player_new(vlcInstance);
            libvlc_media_release(vlcMedia);
            if(vlcPlayer)
            {
                libvlc_media_player_set_xwindow(vlcPlayer, playform->winId());
                libvlc_media_player_play(vlcPlayer);
            }
            //else can't create player
        }
        else qDebug() <<"创建媒体流失败!";
    }
    else qDebug() <<"创建VLC句柄失败!";
    //}}
#else
    qDebug() << "VLC is not available";
#endif
    m_Timer =new QTimer();
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(on_TimerEvent()));
    m_Timer->start(1000);
    m_LearnTime =0;
}

VideoPlay::~VideoPlay()
{
    pVideoWin   =NULL;
    m_IdleTime  =0;
    m_VideoNo   =0;
#ifdef VLC_AVAILABLE
    if(vlcPlayer) libvlc_media_player_release(vlcPlayer);
    if(vlcInstance) libvlc_release(vlcInstance);
#else
#endif
    if(playform) delete playform;

    delete ui;
    if(m_Timer)
    {
        m_Timer->stop();
        delete m_Timer;
    }
    //system("xset s off \n");
    system("xset dpms force on \n");
    //qDebug()<<"Video closed";
}

void VideoPlay::mousePressEvent(QMouseEvent * event)
{
    //qDebug()<<"mouse close";
    this->close();
    Q_UNUSED(event);
}

void VideoPlay::on_TimerEvent()
{
#ifdef VLC_AVAILABLE
    bool loopPlay =true;
#endif
    if(m_VideoNo ==1)
    {
        if(m_LearnTime++ >1200) { this->close(); return; }
    }
    else
    {
        if(shouldDisableFeatureVideo())
        {
            system("xset dpms force off \n");
#ifdef VLC_AVAILABLE
            loopPlay =false;
            if(vlcPlayer) libvlc_media_player_stop(vlcPlayer);
#else
#endif
        }
        else
        {
            //system("xset s off \n");
            system("xset dpms force on \n");    //唤醒
        }
    }
#ifdef VLC_AVAILABLE
    if(vlcPlayer && loopPlay)
    {
        if(!libvlc_media_player_is_playing(vlcPlayer))
        {
            libvlc_media_player_stop(vlcPlayer);
            libvlc_media_player_play(vlcPlayer);
        }
    }
#else
#endif
}

void VideoPlay::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    //qDebug()<<"Key close";
    //if(event->key() !=Qt::Key_Return)
    this->close();
}
