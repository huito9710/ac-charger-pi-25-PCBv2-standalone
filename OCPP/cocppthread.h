#ifndef COCPPTHREAD_H
#define COCPPTHREAD_H

#include <QThread>
#include "ocppclient.h"

class COcppThread : public QThread
{
    Q_OBJECT
public:
    explicit COcppThread(QObject *parent = 0);
    void    stop();
    //void    reConnect();

signals:

public slots:

protected:
    void    run();

private:
    OcppClient *m_pclient=NULL;
    //volatile bool stop;
    volatile bool reconnect;

};

#endif // COCPPTHREAD_H
