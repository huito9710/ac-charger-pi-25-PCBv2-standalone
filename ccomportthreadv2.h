#ifndef CCOMPORTTHREADV2_H
#define CCOMPORTTHREADV2_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QtSerialPort/QSerialPort>
#include "T_Type.h"
#include <QTimer>

class CComPortThreadV2 : public QThread
{
    Q_OBJECT

public:
    //explicit CComPortThreadV2(QObject *parent =nullptr);
    explicit CComPortThreadV2(QSerialPort * serialPort, QObject *parent =nullptr);
    ~CComPortThreadV2();

    void StartSlave(int waitTimeout);
    void SetNewCommand(const QByteArray &command);
    bool verifyRxData(QByteArray &data);

protected:
    void    run();

signals:
    void portOpened(bool succ);
    void ChargerModBus(const QByteArray &array);
    void ChargerModBus2(const CHARGER_RAW_DATA &data);
    void ExtKeyDown(const char keycode);

private slots:
    void handleReadyRead();
    void handleTimeout();
    void handleError(QSerialPort::SerialPortError error);

private:
    QString GetPortName();
    void openSerialPort();
    void LoopChargerData(QSerialPort &serial);

private:
    // FTDI USB-RS485 (Part-No, USB-RS485-PCB)
    static constexpr quint16 venderID = 0x0403;
    static constexpr quint16 productID = 0x6001;
    int     WaitTimeout;
    QMutex  Mutex;
    bool    Stop;
    QByteArray Command;

    QSerialPort * serialPort = nullptr;
    QString comport;
    QByteArray requestData;
    uint8_t req_data_clr_counter = 0;
    QTimer m_timer;
};

#endif // CCOMPORTTHREADV2_H
