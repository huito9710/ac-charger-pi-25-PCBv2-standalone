#include "api_function.h"
#include <QtCore/QDebug>
#include <QFile>
#include <QDateTime>
#include "const.h"
#include "hostplatform.h"

bool GetSetting(const QString& item, QString& value)
{
    bool retcode =false;

    QFile INIfile(QString("%1setting.ini").arg(charger::Platform::instance().settingsIniDir));
    if(INIfile.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        while(!INIfile.atEnd())
        {
            if(QString(INIfile.readLine()).contains(item))
            {
                if(INIfile.atEnd()) break;
                value =INIfile.readLine();
                value.remove('\n');
                value.remove('\r');
                retcode =true;
                break;
            }
        }
        INIfile.close();
    }

    //qDebug() <<value;
    return retcode;
}

//=============================================================================
#define	_C_CRCKEY	0xA001

u16	GetCRC16(u8 data[], int len)
{ //COPY FROM ARM:
    int		i;
    u16		CRC16, flag;
    u8		n;

    CRC16	=0xFFFF;

    for(i=0; i<len; i++)
    {
        CRC16	^=data[i];

        for(n=0; n<8; n++)
        {
            flag	=CRC16 &1;
            CRC16 >>=1;
            CRC16	&=0x7FFF;

            if(flag) CRC16	^=_C_CRCKEY;
        }
    }

    return	CRC16;
}

bool CRC_check(uint8_t data[], int len)
{
    uint16_t crc= GetCRC16(data,len -2);
    uint16_t _crc = data[len-2]*256 + data[len-1];

    return (crc==_crc);
}


//=============================================================================

bool IsValidCard(const QString& CardNo)
{
    bool retcode =false;

    QFile Cardfile(QString("%1Card.lst").arg(charger::Platform::instance().rfidCardsDir));
    if(Cardfile.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        QTextStream ts(&Cardfile);

        while(!ts.atEnd())
        {
            if(CardNo == ts.readLine())
            {
                retcode =true;
                break;
            }
        }
        Cardfile.close();
    }

    return retcode;
}

//=============================================================================

void RemoveOldRecords(QFile &file, const int minNumRecs)
{
    int i =0, rowCounts =0;
    QString linetxt;

    if(file.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        while(!file.atEnd())
        {
            file.readLine();
            rowCounts++;
        }
        file.close();
    }

    if(rowCounts > minNumRecs)
    {
        QFile tmpfile(QString("%1~tmp.csv").arg(charger::Platform::instance().recordsDir));
        if(file.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            if(tmpfile.open(QIODevice::WriteOnly|QIODevice::Text))
            {
                QTextStream record(&tmpfile);
                linetxt =file.readLine();
                record <<linetxt;       //写题头

                while(!file.atEnd())
                {
                    linetxt =file.readLine();
                    if(++i >rowCounts - minNumRecs) record <<linetxt;
                }

                tmpfile.close();
                file.close();

                file.remove();
                tmpfile.rename(file.fileName());
            }
            else file.close();
        }
    }
}

//=============================================================================

//bool IsDateTime()
//{
//    return true;
//}
