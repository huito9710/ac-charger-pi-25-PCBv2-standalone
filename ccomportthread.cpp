#include "ccomportthread.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include "api_function.h"
#include "const.h"
#include "logconfig.h"
#include "distancesensorthread.h"

QT_USE_NAMESPACE

CComPortThread::CComPortThread(QObject *parent)
    : QThread(parent), WaitTimeout(0), Stop(false)
{
}

CComPortThread::~CComPortThread()
{
    Mutex.lock();
    Stop =true;
    Mutex.unlock();
    wait();
}

void CComPortThread::StartSlave(int waitTimeout)
{
    QMutexLocker locker(&Mutex);
    WaitTimeout =waitTimeout;
    Stop =false;
    start();
}

QString CComPortThread::GetPortName()
{
    QString portname("");

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        /*
        qCWarning(ChargerComm) << "Port Name: " << info.portName();
        qCWarning(ChargerComm) << "Description: " << info.description();
        qCWarning(ChargerComm) << "Serial Number: " << info.serialNumber();
        qCWarning(ChargerComm) << "Manufacturer: " << info.manufacturer();
           if (info.hasProductIdentifier())
               qCWarning(ChargerComm) << "Product ID: " << info.productIdentifier();
           if (info.hasVendorIdentifier())
               qCWarning(ChargerComm) << "Vendor ID: " << info.vendorIdentifier();
        */
        if (info.hasVendorIdentifier() && info.hasProductIdentifier()) {
            if (info.vendorIdentifier() == venderID && info.productIdentifier() == productID) {
                portname = info.portName();
                if(DistanceSensorThread::Port !="" && DistanceSensorThread::Port !=portname) break;
                else if(DistanceSensorThread::Port =="") break;
            }
        }
    }
    return portname;
}

void CComPortThread::run()
{
    QString comport;
    QSerialPort serial;

    do
    {
        comport =GetPortName();
        if(comport !="")
        {
            serial.setPortName(comport);
            serial.close();
            serial.setBaudRate(QSerialPort::Baud9600);
            if(serial.open(QIODevice::ReadWrite))
            {
                emit portOpened(true);
                serial.clear();
                LoopChargerData(serial);
                serial.close();
            }
            else
            {
                qCWarning(ChargerComm) << "Can't open com port" << comport;
                emit portOpened(false);
            }
        }
        else {
            qWarning() << "Unable to find suitable Port.";
            emit portOpened(false);
        }

        sleep(3);
    } while(!Stop);
}

void CComPortThread::LoopChargerData(QSerialPort &serial)
{
    do
    {
        if(serial.waitForReadyRead(WaitTimeout))
        {
            QByteArray requestData =serial.readAll();
            while(serial.waitForReadyRead(20))   //5ms(旧OS), 新OS要>16ms
                requestData +=serial.readAll();

            //{{读好了数据:
            qCDebug(ChargerComm) << "SERIAL-READ: (length=" << requestData.length() << ") " << requestData.toHex(' '); //test
            int reqDataSize = requestData.size();
            if((((char)requestData[0]) ==_C_485_ADDR) &&(reqDataSize >=8))
            {
                qCDebug(ChargerComm) << "Address is us";
                u16 crc = GetCRC16(reinterpret_cast<u8*>(requestData.data()), reqDataSize-2);
                u16 expectedCrc = (static_cast<u16>(requestData[reqDataSize-1]) << 8) | (static_cast<u8>(requestData[reqDataSize-2]));
                qCDebug(ChargerComm) <<"GenCRC16 returns 0x" << hex <<  crc << endl;
                qCDebug(ChargerComm) <<"Right-side is 0x" << hex << expectedCrc << endl;
                if(crc == expectedCrc)
                { //CRC正确:
                    if((requestData.data()[1] ==0x03) &&(requestData.data()[2] ==0x00)) //增加测[2]是发现发出的数据也可能被接收
                    { //读命令:
                        qCDebug(ChargerComm) << "MODBUS_READ";
                        QByteArray command =requestData.left(3);
                        char    tempKey =requestData.data()[3];     //利用寄存器地址的低2位传送键状态
                        Mutex.lock();
                        command.append(Command);
                        Mutex.unlock();
                        command.data()[2] =command.size()-3;
                        u16 crc =GetCRC16((u8 *)command.data(), command.size());
                        command.append(crc &0xFF);
                        command.append((crc>>8) &0xFF);
                        serial.write(command);
                        //{处理按键状态:
                        qCDebug(ChargerComm) <<QString("tmpkey:%1").arg((int)tempKey,0, 16)<<"\n";//----
                        if(tempKey &1)
                            emit this->ExtKeyDown('\n');
                        else if(tempKey &2)
                            emit this->ExtKeyDown('M');
                        if(!serial.waitForBytesWritten(50)) {
                            qCWarning(ChargerComm) << "Damn! Serial Port Write timeout";
                            break;
                        }
                        qCDebug(ChargerComm) <<"SERIAL-WRITE Command: (length=" << command.length() << ") " << command.toHex(' ').toUpper(); //---
                    }
                    else if((requestData.data()[1] ==0x10) &&(reqDataSize >8))   //增加测长度解决发出的数据也可能被接收
                    { //写状态:
                        qCDebug(ChargerComm) << "MODBUS_WRITE";
                        // the following calculation should match STM32 RS485_NewSend()
                        if(reqDataSize==requestData.data()[6]+13) {
                            emit this->ChargerModBus(requestData);
                            qCDebug(ChargerComm) << "UART: ChargerModBus emitted!" << endl;
                        }
                        else {
                            qCWarning(ChargerComm) << "UART: unexpected requestData size!" << endl;
                        }
                        QByteArray SendData =requestData.left(8);
                        u16 crc =GetCRC16((u8 *)SendData.data(), SendData.size()-2);
                        SendData.data()[6] =crc &0xFF;
                        SendData.data()[7] =(crc>>8) &0xFF;
                        serial.write(SendData);
                        if(!serial.waitForBytesWritten(50)) {
                            qCWarning(ChargerComm) << "Damn! Serial Port Write timeout";
                            break;
                        }
                        qCDebug(ChargerComm) <<"SERIAL-WRITE Reply: (length=" << SendData.length() << ") " << SendData.toHex(' ').toUpper(); //---
                    }
                    else {
                        // Seems to be reading back what's written in the 2 cases above
                        qCDebug(ChargerComm) << "Unhandled Function: " << QString("0x%1").arg((int)requestData.at(1), 0, 16) << "; DataSize: " << reqDataSize;
                        qCDebug(ChargerComm) << "Full-Data: " << requestData.toHex(' ');
                    }
                }
                else
                {
                    qCCritical(ChargerComm) << "UART: CRC mismatch!" << endl;
                }
            }
            else {
                qCDebug(ChargerComm) << "UART: Either not destined for us or data size not large enough" << endl;
            }
            //}}
        }
        else break;
    }while(!Stop);
}

#include <QTime>
void CComPortThread::SetNewCommand(const QByteArray &command)
{
    QMutexLocker locker(&Mutex);
    qCDebug(ChargerMessages) << QTime::currentTime().toString() << " SetNewCommand: " << Command.toHex().toUpper() <<"size:"<<Command.size();
    Command =command;
    //qCDebug(ChargerComm) <<"UART command: "<<Command.toHex().toUpper() <<"size:"<<Command.size();
}
