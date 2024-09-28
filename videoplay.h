#ifndef VIDEOPLAY_H
#define VIDEOPLAY_H

#include <QMainWindow>
#include <qwidget.h>
#include <QProcessEnvironment>
#include "T_Type.h"
#include <qtimer.h>
#include <QFile>
#ifdef VLC_AVAILABLE
#include <vlc/vlc.h>
#else
#endif

namespace Ui {
class VideoPlay;
}

class VideoPlay : public QMainWindow
{
    Q_OBJECT

public:
    static const char defaultTutorialVideo[];
    static QString tutorialsFile();
    static bool tutorialsFilePresent() { return QFile::exists(VideoPlay::tutorialsFile()); };
    static QString featuredVideoFile();
    static bool shouldDisableFeatureVideo();
    static bool withinAllowedTime(const QTime &curr, const QTime &start, const QTime &end);

public:
    explicit VideoPlay(QWidget *parent = 0);
    ~VideoPlay();

private slots:
    void on_TimerEvent();

protected:
    void mousePressEvent(QMouseEvent * event);
    void keyPressEvent(QKeyEvent *event);

private:
    Ui::VideoPlay *ui;
    QWidget  *playform =NULL;
    QProcess *process =NULL;    //xxx
    QTimer   *m_Timer;
    int      m_LearnTime;

#ifdef VLC_AVAILABLE
    libvlc_instance_t       *vlcInstance =NULL;
    libvlc_media_player_t   *vlcPlayer =NULL;
#else
#endif
};

#endif // VIDEOPLAY_H
