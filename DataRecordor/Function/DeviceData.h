#ifndef DEVICEDATA_H
#define DEVICEDATA_H
#include <QString>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include "data.h"

//长包指令
#define Gunner_Ctrol_LANG_ID         0x1CEC6889
//炮长设置属性指令
#define Gunner_Property_PGN          0x00FF04
//炮长获取射击数据
#define Gunner_FiringData_PGN        0x00FF03

enum CommandCodeWord
{
    CMD_Code_SysTimeSet     =0x07,
    CMD_Code_Request        =0xC3,
    CMD_Code_Report         =0xC4,
    CMD_Code_SetAttribute   =0x01B0
};

enum Device_Stat
{
    Device_Stat_DisLink =0,
    Device_Stat_Normal  =0x0F,
    Device_Stat_Fault   =0xFF
};

enum DataFlagToSend
{
    DataFlag_Attribute      =0,
    DataFlag_WorkStat       =1,
    DataFlag_AlarmInfo      =2,
    DataFlag_TotalWorkTime  =3,
    DataFlag_GunMoveData    =4,
    DataFlag_GunshootData   =5,
    DataFlag_AllData        =0xFF,
};

struct CanSignal {
    QString name    ;// 信号名称
    int startBit    ;// 起始位
    int length      ;// 长度
    QString unit    ;// 单位
    QString dataType;// 数据类型
    double factor   ;// 比例因子
    double offset   ;// 偏移量
    QMap<int, QString> valueTable; // 值表
};

struct CanDataFormat {
    int id                      ;// CAN ID
    QString msgName             ;// 消息名称
    QList<CanSignal> canSignals ;// 信号列表
};

struct CanDataValue {
    QVariant value          ;
    QString valueDescription;//描述
    QString unit            ;//单位
};

#pragma pack(1)  //内存1字节对齐

typedef struct
{
    unsigned short ti_year  ;//1900-2100
    unsigned char  ti_mon   ;//1-12
    unsigned char  ti_day   ;//1-31
    unsigned char  ti_hour  ;//0-23
    unsigned char  ti_min   ;//0-59
    unsigned char  ti_sec   ;//0-59
    unsigned char  ti_hund  ;//用毫秒/10表示 0-99
} LongDateTime;

struct SelfAttributeData {
    quint8 attribute   ;// 本车属性
    char uniqueID[10]    ;// 本车唯一ID
};

struct DeviceStatus {
    quint8 deviceAddress;// 设备地址
    quint8 Status       ;// 设备状态
    quint8 faultInfo[7] ;// 设备故障信息
};

struct DeviceStatusInfo {
    LongDateTime dateTime       ;
    DeviceStatus deviceStatus   ;
};

struct AlarmInfo {
    quint8 alarmContent             ;// 报警内容
    LongDateTime statusChangeTime   ;// 报警状态变化时刻
    quint16 alarmDetails            ;// 报警信息
};

struct GunMoveData {
    quint16 barrelDirection         ;// 身管方向
    quint16 elevationAngle          ;// 俯仰角
    quint16 chassisRoll             ;// 底盘横倾姿态数据
    quint16 chassisPitch            ;// 底盘纵倾姿态数据
    LongDateTime statusChangeTime   ;// 状态变化时刻
    quint8 autoAdjustmentStatus     ;// 自动调炮状态
};

struct GunFiringData {
    quint16 barrelDirection         ;// 身管方向
    quint16 elevationAngle          ;// 俯仰角
    quint16 chassisRoll             ;// 底盘横倾姿态数据
    quint16 chassisPitch            ;// 底盘纵倾姿态数据
    LongDateTime statusChangeTime   ;// 状态变化时刻
    quint8 firingCompletedSignal    ;// 击发完毕信号
    quint8 recoilStatus             ;// 复进状态
    quint8 muzzleVelocityValid      ;// 初速有效标识
    quint16 propellantTemperature    ;// 药温
    quint16 muzzleVelocity          ;// 初速信息
};

struct DeviceTotalWorkTime {
    int deviceId        ;// 设备ID
    int totalWorkTime   ;// 总工作时间
};

#pragma pack()  //内存1字节对齐

struct DeviceName {
    quint8 deviceAddress    ;// 设备地址
    QString deviceName      ;// 设备名称
};
struct DeviceErrorInfo {
    int statId          ;// 关联的状态ID
    QString errorInfo   ;// 故障信息
};

class TimeCondition
{
public:
    TimeCondition(QDateTime _startTime,QDateTime _endTime)
        :startTime(_startTime),endTime(_endTime)
    {
    }
    QDateTime startTime;
    QDateTime endTime;
};

class TimeFormatTrans
{
public:
    //时间转换函数
    static QDateTime getDateTime(LongDateTime dateTime)
    {
        return  QDateTime(QDate(dateTime.ti_year, dateTime.ti_mon, dateTime.ti_day),
                          QTime(dateTime.ti_hour, dateTime.ti_min, dateTime.ti_sec, dateTime.ti_hund * 10));
    }

    static LongDateTime getLongDataTime(QDateTime timestamp)
    {
        // 将 QDateTime 转换为 LongDateTime
        LongDateTime dateTime;
        dateTime.ti_year = timestamp.date().year();
        dateTime.ti_mon = timestamp.date().month();
        dateTime.ti_day = timestamp.date().day();
        dateTime.ti_hour = timestamp.time().hour();
        dateTime.ti_min = timestamp.time().minute();
        dateTime.ti_sec = timestamp.time().second();
        dateTime.ti_hund = timestamp.time().msec() / 10;
        return dateTime;
    }
    // 将 LocalDateTime 转换为 LongDateTime
    static LongDateTime convertToLongDateTime(const LocalDateTime& localDateTime)
    {
        LongDateTime longDateTime;
        longDateTime.ti_year = localDateTime.year;
        longDateTime.ti_mon = localDateTime.month;
        longDateTime.ti_day = localDateTime.day;
        longDateTime.ti_hour = localDateTime.hour;
        longDateTime.ti_min = localDateTime.minute;
        longDateTime.ti_sec = localDateTime.second;
        longDateTime.ti_hund = localDateTime.msec / 10; // 将毫秒转换为毫秒/10
        return longDateTime;
    }

};


#endif // DEVICEDATA_H
