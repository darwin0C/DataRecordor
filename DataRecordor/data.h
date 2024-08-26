#ifndef DATA_H
#define DATA_H

#include <QStringList>
#include <QDataStream>
const QString DBDATEFileName="/data/dataBase.db" ; //数据库存储文件
const QString DeviceNameFile="/settings/deviceName.cdb" ; //设备名称
const QString DeviceStat_CANDataFile="/settings/CAN_DeviceStat.cdb" ; //总线协议
const QString Event_CANDataFile="/settings/CAN_Event.cdb" ; //总线协议

const QString GlobSettingFile="/settings/setting.ini" ; //总线协议

const QString gPath="D:/dataSave/";

#pragma pack(1)  //内存1字节对齐

typedef struct
{
    unsigned short year     ;//年
    unsigned char month     ;//月
    unsigned char day       ;//日
    unsigned char hour      ;//时
    unsigned char minute    ;//分
    unsigned char second    ;//秒
    unsigned short msec;//毫秒
    //
}LocalDateTime;

typedef struct
{
    LocalDateTime dateTime  ;
    unsigned char   len     ; //
    unsigned int    dataid  ;
    unsigned char   data[8] ; //8字节的数据

}CanData;

typedef struct
{
    unsigned char head      ;//帧头	0XC1
    unsigned char flag      ;//标志	0x0A/0x0B   0A：存储数据；0B：指令（也存储）
    unsigned char port      ;//CAN口号	1-3

    CanData candata         ;//can结构体
    unsigned char checkCode;//校验码 字节和校验  低8位
    //
}SerialDataRev;


typedef struct
{
    unsigned char head        ;//帧头	0XD1
    unsigned char equStat     ;//存储板设备状态	0x0F 正常，0xFF 故障
    unsigned char sdCardStat  ;//SD卡状态	0x0F 正常，0xFF 故障
    unsigned int sdCardCapcity;//SD卡剩余容量	KB
    unsigned char usedPercentage;//SD 卡使用容量百分比	1~100%
    unsigned char rev[3];
    unsigned char checkCode;//校验码 字节和校验  低8位
    //
}SerialDataSend;

#pragma pack()

const QString ledGreen_on = "echo 1 > /sys/class/gpio/gpio1/value";
const QString ledRed_on = "echo 1 > /sys/class/gpio/gpio2/value";
const QString ledYellow_on="echo 1 > /sys/class/gpio/gpio1/value;echo 1 > /sys/class/gpio/gpio2/value";
const QString led_off = "echo 0 > /sys/class/gpio/gpio1/value;echo 0 > /sys/class/gpio/gpio2/value";

#endif // DATA_H

