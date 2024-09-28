#ifndef CCOMPORTTHREAD_H
#define CCOMPORTTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QtSerialPort/QSerialPort>

class CComPortThread : public QThread
{
    Q_OBJECT

public:
    CComPortThread(QObject *parent =0);
    ~CComPortThread();

    void StartSlave(int waitTimeout);
    void SetNewCommand(const QByteArray &command);

protected:
    void    run();

signals:
    void portOpened(bool succ);
    void ChargerModBus(const QByteArray &array);
    void ExtKeyDown(const char keycode);

private:
    QString GetPortName();
    void LoopChargerData(QSerialPort &serial);

private:
    // FTDI USB-RS485 (Part-No, USB-RS485-PCB)
    static constexpr quint16 venderID = 0x0403;
    static constexpr quint16 productID = 0x6001;
    int     WaitTimeout;
    QMutex  Mutex;
    bool    Stop;
    QByteArray Command;
};

#endif // CCOMPORTTHREAD_H
