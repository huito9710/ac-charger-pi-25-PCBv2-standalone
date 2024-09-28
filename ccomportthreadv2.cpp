#include "ccomportthreadv2.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QSharedPointer>
#include "api_function.h"
#include "const.h"
#include "logconfig.h"
#include "distancesensorthread.h"

QT_USE_NAMESPACE

CComPortThreadV2::CComPortThreadV2(QSerialPort * sPort,QObject *parent)
    : QThread(parent), WaitTimeout(0), Stop(false),
      serialPort(sPort)
{
    connect(serialPort, &QSerialPort::readyRead, this, &CComPortThreadV2::handleReadyRead);
    connect(serialPort, &QSerialPort::errorOccurred, this, &CComPortThreadV2::handleError);
    connect(&m_timer, &QTimer::timeout, this, &CComPortThreadV2::handleTimeout);

    openSerialPort();
    m_timer.start(200);
}

CComPortThreadV2::~CComPortThreadV2()
{
    Mutex.lock();
    Stop =true;
    Mutex.unlock();
    wait();
}

void CComPortThreadV2::StartSlave(int waitTimeout)
{
    QMutexLocker locker(&Mutex);
    WaitTimeout =waitTimeout;
    Stop =false;
    start();
}

QString CComPortThreadV2::GetPortName()
{
    QString portname("");

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        qWarning() <<"COM port:"<< info.portName();
        if (info.hasVendorIdentifier() && info.hasProductIdentifier()) {
            if (info.vendorIdentifier() == venderID && info.productIdentifier() == productID) {
                portname = info.portName();
                //if(DistanceSensorThread::Port !="" && DistanceSensorThread::Port !=portname) break;
                //else if(DistanceSensorThread::Port =="") break;
                break;
            }
        }
    }
    return portname;
}
void CComPortThreadV2::handleReadyRead()  {

    QByteArray tempData =serialPort->readAll();
    requestData.append(tempData);

    //{{读好了数据:
    qWarning(ChargerComm) << "SERIAL-READ: (length=" << requestData.length() << ") " << requestData.toHex(' '); //test

    if(verifyRxData(requestData)) {

        QByteArray typeByteArr = QByteArray(requestData.constData(),2);
        uint16_t type = *reinterpret_cast<const uint16_t*>(typeByteArr.data());

        switch(type){

        case 0x0201:{
            QByteArray cmdRaw = QByteArray(requestData.constData()+4,requestData.size()-6);

            emit this->ChargerModBus(cmdRaw);

            QByteArray keyByte = QByteArray(requestData.constData()+6, 1);
            char tempKey = keyByte.data()[0];
            //{处理按键状态:
            //qCDebug(ChargerComm) <<QString("tmpkey:%1").arg((int)tempKey,0, 16)<<"\n";//----
            qWarning(ChargerComm) <<QString("tmpkey:%1").arg((int)tempKey,0, 16)<<"\n";//----
            if(tempKey &1)
                emit this->ExtKeyDown('\n');
            else if(tempKey &2)
                emit this->ExtKeyDown('M');

            }
            break;
        }

        requestData.clear();
    }

    if (!m_timer.isActive())
        m_timer.start(200);
}

void CComPortThreadV2::openSerialPort()  {

    comport =GetPortName();
    if(comport !="")
    {
        serialPort->setPortName(comport);
        serialPort->setBaudRate(QSerialPort::Baud115200);
        serialPort->setStopBits(QSerialPort::StopBits::OneStop);
        serialPort->setParity(QSerialPort::Parity::NoParity);
        serialPort->setDataBits(QSerialPort::DataBits::Data8);
        serialPort->setFlowControl(QSerialPort::FlowControl::NoFlowControl);

        if(serialPort->open(QIODevice::ReadWrite))
        {
            emit portOpened(true);
        } else  {
            qCWarning(ChargerComm) <<"COM port:"<< "Can't open com port" << comport;
            emit portOpened(false);
        }
    }
}

void CComPortThreadV2::handleTimeout()  {

    if((++req_data_clr_counter) == 10){

        requestData.clear();
        req_data_clr_counter =0;
    }

    if(Command.size()>0)  {
        if(! serialPort->isOpen()) this->openSerialPort();

        serialPort->clear();

        QByteArray cmdToSend;
        Mutex.lock();
        cmdToSend.append(0x01);
        cmdToSend.append(0x02);
        uint16_t len = static_cast<uint16_t>(Command.size());
        cmdToSend.append(QByteArray(reinterpret_cast<char *>(&len),2));
        cmdToSend.append(Command);
        Mutex.unlock();
        u16 crc =GetCRC16(reinterpret_cast<u8 *>(cmdToSend.data()), cmdToSend.size());
        cmdToSend.append((crc>>8) &0xFF);
        cmdToSend.append(crc &0xFF);

        serialPort->write(cmdToSend);

        qWarning(ChargerComm) << "SERIAL-WRITE: (length=" << cmdToSend.length() << ") " << cmdToSend.toHex(' '); //test
    }
}

void CComPortThreadV2::handleError(QSerialPort::SerialPortError serialPortError)  {
    if (serialPortError == QSerialPort::ReadError) {
        qDebug() << QObject::tr("An I/O error occurred while reading "
                                        "the data from port %1, error: %2")
                            .arg(serialPort->portName())
                            .arg(serialPort->errorString())
                         << endl;
        //QCoreApplication::exit(1);
    }
}
void CComPortThreadV2::run()
{
    /*
    QSerialPort serial;
    do
    {
        comport =GetPortName();
        if(comport !="")
        {
            serial.setPortName(comport);
            serial.close();
            serial.setBaudRate(QSerialPort::Baud115200);
            serial.setStopBits(QSerialPort::StopBits::OneStop);
            serial.setParity(QSerialPort::Parity::NoParity);
            serial.setDataBits(QSerialPort::DataBits::Data8);
            serial.setFlowControl(QSerialPort::FlowControl::NoFlowControl);

            if(serial.open(QIODevice::ReadWrite))
            {
                emit portOpened(true);
                serial.clear();
                LoopChargerData(serial);
                serial.close();
            }
            else
            {
                qCWarning(ChargerComm) <<"COM port:"<< "Can't open com port" << comport;
                emit portOpened(false);
            }
        }
        else {
            qWarning() <<"COM port:"<< "Unable to find suitable Port.";
            emit portOpened(false);
        }

        sleep(3);
    } while(!Stop);
*/
}

void CComPortThreadV2::LoopChargerData(QSerialPort &serial)
{
    do
    {
        if(Command.size()>0)  {
            QByteArray cmdToSend;
            Mutex.lock();
            cmdToSend.append(0x01);
            cmdToSend.append(0x02);
            uint16_t len = static_cast<uint16_t>(Command.size());
            cmdToSend.append(QByteArray(reinterpret_cast<char *>(&len),2));
            cmdToSend.append(Command);
            Mutex.unlock();
            u16 crc =GetCRC16(reinterpret_cast<u8 *>(cmdToSend.data()), cmdToSend.size());
            cmdToSend.append((crc>>8) &0xFF);
            cmdToSend.append(crc &0xFF);
            serial.write(cmdToSend);

            qWarning(ChargerComm) << "SERIAL-WRITE: (length=" << cmdToSend.length() << ") " << cmdToSend.toHex(' '); //test
        }

        if(!serial.waitForBytesWritten(200)) {
            qCWarning(ChargerComm) << "Damn! Serial Port Write timeout";
            break;
        }

        if(serial.waitForReadyRead(WaitTimeout))
        {
            QByteArray requestData =serial.readAll();
            while(serial.waitForReadyRead(100))   //5ms(旧OS), 新OS要>16ms
                requestData +=serial.readAll();

            //{{读好了数据:
            //qCDebug(ChargerComm) << "SERIAL-READ: (length=" << requestData.length() << ") " << requestData.toHex(' '); //test
            qWarning(ChargerComm) << "SERIAL-READ: (length=" << requestData.length() << ") " << requestData.toHex(' '); //test
            //int reqDataSize = requestData.size();

            if(verifyRxData(requestData)) {
                QByteArray typeByteArr = QByteArray(requestData.constData(),2);
                uint16_t type = *reinterpret_cast<const uint16_t*>(typeByteArr.data());

                switch(type){

                case 0x0201:{
                    QByteArray cmdRaw = QByteArray(requestData.constData()+4,requestData.size()-6);

                    emit this->ChargerModBus(cmdRaw);

                    QByteArray keyByte = QByteArray(requestData.constData()+6, 1);
                    char tempKey = keyByte.data()[0];
                    //{处理按键状态:
                    //qCDebug(ChargerComm) <<QString("tmpkey:%1").arg((int)tempKey,0, 16)<<"\n";//----
                    qWarning(ChargerComm) <<QString("tmpkey:%1").arg((int)tempKey,0, 16)<<"\n";//----
                    if(tempKey &1)
                        emit this->ExtKeyDown('\n');
                    else if(tempKey &2)
                        emit this->ExtKeyDown('M');

                    }
                    break;
                }

            }else {
                qCCritical(ChargerComm) << "UART: CRC mismatch!" ;
                //qCDebug(ChargerComm) << "UART: Either not destined for us or data size not large enough" ;
                qWarning(ChargerComm) << "UART: Either not destined for us or data size not large enough" ;
            }
            //}}
        }
        else break;

        QThread::sleep(1);
    }while(!Stop);
}

bool CComPortThreadV2::verifyRxData(QByteArray &requestData)
{
    if(requestData.length() <6 ){
        return false;
    }
    int reqDataSize = requestData.size();
    u16 crc = GetCRC16(reinterpret_cast<u8*>(requestData.data()), reqDataSize-2);
    u16 expectedCrc = static_cast<u16>((static_cast<u16>(requestData[reqDataSize-2]) << 8) | (static_cast<u8>(requestData[reqDataSize-1])));

    if(crc != expectedCrc){
        return false;
    }
    return true;
}

#include <QTime>
void CComPortThreadV2::SetNewCommand(const QByteArray &command)
{
    QMutexLocker locker(&Mutex);
    qCDebug(ChargerMessages) << QTime::currentTime().toString() << " SetNewCommand: " << Command.toHex().toUpper() <<"size:"<<Command.size();
    Command =command;
    //qCDebug(ChargerComm) <<"UART command: "<<Command.toHex().toUpper() <<"size:"<<Command.size();
}
