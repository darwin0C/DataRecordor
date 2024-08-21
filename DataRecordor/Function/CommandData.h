#ifndef COMMANDDATA_H
#define COMMANDDATA_H

#include "DeviceData.h"
#pragma pack(1)  //内存1字节对齐


typedef struct
{
    quint8 commandCod       ;//命令字
    quint8 rev[48]          ;//其它信息	48字节
    unsigned char  ti_min   ;//0-59
    unsigned char  ti_hour  ;//0-23
    unsigned char  ti_sec   ;//0-59
    unsigned char  ti_hund  ;//用毫秒/10表示 0-99

    unsigned short ti_year  ;//1900-2100
    unsigned char  ti_day   ;//1-31
    unsigned char  ti_mon   ;//1-12
} TimeSetCMD;

typedef struct
{
    quint8 commandCod       ;//命令字
    quint8 msgReportCtrl    ;//信息上报控制
    quint8 requreData       ;//查询内容
    quint8 deviceAddress    ;//设备地址
    quint8 requreMethod     ;//查询方式
    LongDateTime startTime  ;//开始时间
    LongDateTime endTime    ;//结束时间
} CommandDataRequre;

typedef struct
{
    quint16 selfAttribute   ;//本车属性
    quint32 selfUniqueID    ;//本车唯一ID
    quint8 dataFlag         ;//数据标识
    quint8 dataType         ;//数据类型
    quint8 dataPacketIndedx ;//数据包序号
} Send2CommandData;




#pragma pack()

#endif // COMMANDDATA_H
