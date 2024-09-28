#ifndef CONST_H
#define CONST_H

#define     LED_PIN     18      //灯脚
#define     IR_TxPIN    18      //红外发送脚

//PCB COMMAND:
#define     _C_LOCKTIMEOUT      6       //3"
#define     _C_LOCKCOMMAND      0x01
#define     _C_LOCKCTRL         0x02
#define     _C_CHARGEMASK       0x1C
#define     _C_CHARGECOMMAND    0x08    //充电 发电量 时间
#define     _C_CHARGEDT         0x0C    //充电 发开始日期时间
#define     _C_CHARGESTOP       0x04
#define     _C_CHARGEIDLE       0x10    //取消充电
#define     _C_BOOKINGCOMMAND   (0x7<<2)//预约命令

//PCB STATE:
#define     _C_STATE_CONTACTORB 0x8000  //接触器反馈
#define     _C_STATE_ESTOP      0x4000  //急停
#define     _C_STATE_LOCK       0x2000  //锁反馈
#define     _C_STATE_EARTH      0x1000  //接地故障
#define     _C_STATE_DIODE      0x0800  //CP线车端无二极管
#define     _C_STATE_CONTACTOR  0x0400  //接触器故障
#define     _C_STATE_VOLTAGE    0x0200  //过压或欠压
#define     _C_STATE_CURRENT    0x0100  //过流
#define     _C_STATE_FAULT      0x5F83  //83: MCB--- --SURGE RCCB 漏电

#define     _C_STATE_NOCHARGE   0       //PCB右移了16位
#define     _C_STATE_CHARGEING  1
#define     _C_STATE_FINISH     2
#define     _C_STATE_BOOKING    7
#define     _C_STATE_CHARGEMASK 0xF0000 //绝对位置
#define     _C_STATE_ENABLE     0x1000

// This corresponds to Const.h of ac-charger-st project
constexpr   int _OCPP_STATE_UNKNOWN = 0;
constexpr   int _OCPP_STATE_A = 1;
constexpr   int _OCPP_STATE_B = 2;
constexpr   int _OCPP_STATE_C = 3;
constexpr   int _OCPP_STATE_D = 4;
constexpr   int _OCPP_STATE_E = 5;
constexpr   int _OCPP_STATE_F = 6;
constexpr   int _OCPP_STATE_FAULT = 8;

#define     _C_485_ADDR         10

#endif // CONST_H
