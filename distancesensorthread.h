#ifndef DISTANCESENSORTHREAD_H
#define DISTANCESENSORTHREAD_H

#include <QThread>
#include <QMutex>
#include <QtSerialPort/QSerialPort>
#include <QWaitCondition>
#include <QTimer>
#include "T_Type.h"

class DistanceSensorThread : public QThread
{
    Q_OBJECT

public:
    DistanceSensorThread(QObject *parent =nullptr);
    ~DistanceSensorThread();

    void StartSlave(int waitTimeout);
    void StartSlave(const QString &port, int waitTimeout);
    static float GetDistance(void);
    static bool checkSerialPortName(const QString &port);

    static QString Port;
protected:
    void    run();
    QString getPortName();

signals:
    void   parkingStateChanged(QString pState, float dist);

private:
    // FTDI USB-RS485 (Part-No, USB-RS485-PCB)
    static constexpr quint16 venderID = 0x0403;
    static constexpr quint16 productID = 0x6001;
    static QList<float> DistList;

    QSerialPort serial;
    int     WaitTimeout;
    QMutex  Mutex;
    int     Read;
    char    Status;     //0:OK 1:串口故障 2:无读卡器
    bool    Stop;
};

static bool variantLessThan(const float &v1, const float &v2)
{
    return v1 < v2;
}

#endif // DISTANCESENSORTHREAD_H
