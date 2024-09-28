#ifndef CCOMPORTMASTERTHREAD_H
#define CCOMPORTMASTERTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include "distancesensorthread.h"

class CComPortMasterThread : public QThread
{
    Q_OBJECT

public:
    explicit CComPortMasterThread(QObject *parent = nullptr);
    ~CComPortMasterThread();

    void transaction(const QString &portName, int waitTimeout, const QString &request);

signals:
    void response(const QString &s);
    void error(const QString &s);
    void timeout(const QString &s);
    void portOpened(bool succ);
    void ChargerModBus(const QByteArray &array);
    void ExtKeyDown(const char keycode);

private:
    QString getPortName();
    void LoopChargerData(QSerialPort &serial);
    void run() override;

    // FTDI USB-RS485 (Part-No, USB-RS485-PCB)
    static constexpr quint16 venderID = 0x0403;
    static constexpr quint16 productID = 0x6001;
    QString m_portName;
    QString m_request;
    int m_waitTimeout = 0;
    QMutex m_mutex;
    QWaitCondition m_cond;
    bool m_quit = false;
};


#endif // CCOMPORTMASTERTHREAD_H
