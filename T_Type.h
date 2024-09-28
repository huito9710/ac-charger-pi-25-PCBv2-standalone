#ifndef T_TYPE_H
#define T_TYPE_H
#include <QtCore/QString>
#include <QDateTime>

#define u8      uchar
#define U8      u8
#define u16     uint16_t
#define U16     u16
#define u32     uint32_t
#define U32     u32

struct CHARGER_RAW_DATA   //Read ST-PCB Command Response (type:0x0201)
{
    u32 State;      //CP:u8(EN_CP) Charge:u8(停充原因_命令) Fault:u16     停充原因:0无 2故障 3拔线 4开锁
    u8  CableMaxCurrent;
    u8  MaxCurrent; // Actual Maximum current being applied
    u8  keyTrig;
    u8  FirmwareVer;
    u32 Time;       //剩余时间
    u32 Q;
    u32 Power;
    u16 PFactor;
    u16 Frequency;
    u16 Reserved1;
    u16 Va;
    u16 Vb;
    u16 Vc;
    u32 Ia;
    u32 Ib;
    u32 Ic;
};

struct CHARGER_RAW_DATA_EXT  //Read Pi Command Request (type:0x0301)
{
    u8 keyTriggered;
};

struct CHARGE_COMMAND        //Read Pi Command Response (type:0x0301)
{
    u8 commandBody;
    u8 connectorType;
    u8 limitedCurrent;
    u32 remainTime;
    u32 chargeQValue;
    char dateStr[8];
    u16 meterType;
    u16 ledState;
};

#define TYPE_BOOL	1
#define TYPE_CHAR	2
// ......
#define TYPE_TLV_	12
//Pi-PCB TxRx Protocol Command Codes,eg. 0x0201, 0x0301

struct CHARGE_RECORD_TYPE
{
    static constexpr int MaxIdLength = 20;
    u32 Time;
    u32 MeterStartVal;
    u32 MeterStopVal;
    u32 Q;
    u32 RemainTime;
    u32 RemainQ;
    int TransactionId;
    u8  ConnectorId;
    u8  lastChargingProfile; // max-current
    u8  Status;
    char    StartDt[24];
    char    StopDt[24];
    uchar   SocketType;
    u8      IdLength; // number of bytes for the ID field
    char    ID[MaxIdLength];   // OCPPv1.6 supports 20 bytes ID
    char    Pwd[5];
    bool    isResumedCharge;
};

struct EvChargingDetailMeterVal
{
    QDateTime meteredDt;
    unsigned long Q;
    unsigned long P;
    unsigned long I[3];
    unsigned long V[3];
    unsigned long Pf;
    unsigned long Freq;
};

struct RESERVE_RECORD_TYPE
{
    static constexpr int MaxIdLength = 20;
    u8         ReservationId;
    u8      IdLength; // number of bytes for the ID field
    char    ID[MaxIdLength];   // OCPPv1.6 supports 20 bytes ID
    char   ExpiryDate[24];
};

#endif // T_TYPE_H
