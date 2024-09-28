#include "distancesensorthread.h"
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QtCore/QDebug>
#include "api_function.h"
#include "const.h"
#include "logconfig.h"
#include "chargerconfig.h"

//[用法:
//DistanceSensorThread    distSensThread;
//DistanceSensorThread.StartSlave("/dev/ttyS0", 100);
//DistanceSensorThread.GetDistance();
//]

QT_USE_NAMESPACE

QList<float> DistanceSensorThread::DistList = QList<float>();

//! [constructor]
DistanceSensorThread::DistanceSensorThread(QObject *parent)
    : QThread(parent), WaitTimeout(100), Stop(false)
{
    //this->setPriority(QThread::InheritPriority);
}
//! [constructor]

QString DistanceSensorThread::Port = "";

DistanceSensorThread::~DistanceSensorThread()
{
    Mutex.lock();
    Stop =true;         //控制结束,清理线程
    Mutex.unlock();
    Read =0;
    wait();
    qCDebug(Rfid) << "Distance Sensor Exit";
}

void DistanceSensorThread::StartSlave(int waitTimeout)
{
    QString portName = getPortName();
    if(portName == "") return;

    QMutexLocker locker(&Mutex);
    DistanceSensorThread::Port = portName;       // /dev/ttyS0 ?
    WaitTimeout =waitTimeout;
    Stop    =false;
    Read    =0;
    Status  =1;
    start();
}

void DistanceSensorThread::StartSlave(const QString &port, int waitTimeout)
{
    if(port == "") return;

    QMutexLocker locker(&Mutex);
    DistanceSensorThread::Port =port;         // /dev/ttyS0 ?
    WaitTimeout =waitTimeout;
    Stop    =false;
    Read    =0;
    Status  =1;
    start();
    //start(QThread::HighPriority);
}

void DistanceSensorThread::run()
{
    int senseTimes = charger::GetDistSenseTimes();
    int distThreshold = charger::GetDistThreshold();

    serial.setPortName(DistanceSensorThread::Port);
    serial.setBaudRate(QSerialPort::Baud9600);    //8 N 1stop
    serial.setDataBits(QSerialPort::Data8);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setParity(QSerialPort::NoParity);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    float dist_0 = 0;
    float dist_1 = 0;
    if(DistanceSensorThread::Port !="")
    {
        serial.close();
        if(serial.open(QSerialPort::ReadWrite)){
            QByteArray incomedata_temp = "";
            do
            {
                QByteArray trigger(1,0x00);
                serial.write(trigger);

                serial.waitForReadyRead(100);
                QByteArray rawdata = serial.readAll();

                int rawlen = rawdata.length();
                //qDebug() <<"Raw Data length:" << rawlen;
                if(rawlen < 4){
                    incomedata_temp = incomedata_temp.append(rawdata);
                    if(incomedata_temp.length()<4)
                        continue;
                    else {
                        rawdata = incomedata_temp;
                        incomedata_temp = "";
                    }
                }
                else{
                    incomedata_temp = "";
                }

                uint8_t data[4] ;
                for(int i=0;i<4;i++){
                    data[i] = (uint8_t)rawdata[i];
                }
                //qWarning() <<"Raw Distance Data:"<< data;
                uint8_t int_high = data[1];
                uint8_t int_low = data[2];
                uint8_t int_sum = data[3] + 1;

                //串口在QT QSerialPort 上有异于产品规范表现，首字节0xFF错位
                if(data[0]!=255){
                    int_high = data[0];
                    int_low = data[1];
                    int_sum = data[2] + 1;
                }

                if(int_high + int_low == int_sum)
                {
                    float dist = ((float)int_high*256+(float)int_low)/340/2;
                    Mutex.lock();
                    DistanceSensorThread::DistList.push_back(dist);
                    while(DistanceSensorThread::DistList.count()>senseTimes){ //保留10个最近测算的距离值
                        DistanceSensorThread::DistList.pop_front();
                    }
                    Mutex.unlock();
                }
                //qWarning() <<"Distance List Count:"<< DistanceSensorThread::DistList.count();
                //qWarning() <<"Current Distance:"<< GetDistance();
                if(!Stop) sleep(1);

                dist_1 = DistanceSensorThread::GetDistance();
                if(DistanceSensorThread::DistList.count()>=senseTimes)
                if((dist_0*100 < distThreshold && distThreshold < dist_1*100)
                        || (dist_1*100 < distThreshold && distThreshold < dist_0*100)
                        || (dist_0 ==0 && dist_1 >0)){
                    emit parkingStateChanged(
                           (dist_1*100 < distThreshold)?"occupied":"free"
                                ,dist_1);
                }
                dist_0 = dist_1;
            } while(!Stop);
            serial.close();
        }else{
            qWarning() <<"Open UART port fail!";
        }
    }
}

float DistanceSensorThread::GetDistance(void)
{
    int senseTimes = charger::GetDistSenseTimes();

    float res;
    //Mutex.lock();
    QList<float> reslist = DistanceSensorThread::DistList;
    //Mutex.unlock();
    std::sort(reslist.begin(), reslist.end(),variantLessThan);
    if(senseTimes > 3){
        while(reslist.count()> senseTimes/3 ){
            reslist.pop_front();
            if(reslist.count()> senseTimes/3 ) reslist.pop_back();
        }
    }

    if(reslist.count()>1){
        float resTotal=0;
        foreach (const float &resItem, reslist){
            resTotal += resItem;
        }
        res = resTotal/reslist.count();
    }
    else if(reslist.count()>0)
        res = reslist[0];
    else
        res = 0;
    return res;
}

QString DistanceSensorThread::getPortName()
{
    QString portname("");
    int _t = 3;  //try 3 times
    while(_t>0){
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        {
            if (info.hasVendorIdentifier() && info.hasProductIdentifier()) {
                if (info.vendorIdentifier() == venderID && info.productIdentifier() == productID) {
                    portname = info.portName();
                    if(checkSerialPortName(portname))
                        return portname;
                }
            }
        }
        _t--;
    }
    return QString("");
}

bool DistanceSensorThread::checkSerialPortName(const QString &port)
{
    QSerialPort serial;
    serial.setPortName(port);
    serial.setBaudRate(QSerialPort::Baud9600);    //8 N 1stop
    serial.setDataBits(QSerialPort::Data8);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setParity(QSerialPort::NoParity);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    if(port !="")
    {
        if(serial.open(QSerialPort::ReadWrite)){
            QByteArray trigger(1,0x00);
            serial.write(trigger);

            serial.waitForReadyRead(300);
            QByteArray rawdata = serial.readAll();

            if(rawdata.length() == 4){
                uint8_t int_head = static_cast<uint8_t>(rawdata[0]);
                if(int_head == 255)
                    return true;
            }
            serial.close();
        }
    }
    return false;
}
