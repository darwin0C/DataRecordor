#ifndef DBMANIPULATION_H
#define DBMANIPULATION_H
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <qmap.h>
#include <QSqlError>
#include "dbDatas.h"
#include "data.h"
#include "DeviceData.h"

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

class DbManipulation
{

public:

    QMap<int,QString> dbMap;
    DbManipulation();
    ~DbManipulation();
    void initial(QString path);
    QSqlDatabase database;// 用于建立和数据库的连接
    QSqlQuery query;
    bool openDb();
    void closeDb();
    bool isTableExist(QString tableName);
    void createTable(int index);

    bool insertAlarmInfo(const AlarmInfo &alarmInfo);
    bool insertGunMoveData(const GunMoveData &gunMoveData);
    bool insertGunFiringData(const GunFiringData &gunFiringData);
    bool insertDeviceName(const DeviceName &deviceName);
    bool insertDeviceErrorInfo(const DeviceErrorInfo &errorInfo);
    bool insertDeviceStatusInfo(const DeviceStatusInfo &statusInfo);
    bool insertDeviceTotalWorkTime(const DeviceTotalWorkTime &workTimeInfo);

    bool updateAlarmInfo(const AlarmInfo &alarmInfo);
    bool updateGunMoveData(const GunMoveData &gunMoveData);
    bool updateGunFiringData(const GunFiringData &gunFiringData);
    bool updateDeviceStatusInfo(const DeviceStatusInfo &statusInfo);
    bool updateDeviceErrorInfo(const DeviceErrorInfo &errorInfo);
    bool updateDeviceTotalWorkTime(const DeviceTotalWorkTime &workTimeInfo);

    QList<DeviceName> getDeviceNames();
    QList<DeviceErrorInfo> getDeviceErrorInfos(int statId);
    QList<DeviceTotalWorkTime> getDeviceTotalWorkTimes(int deviceId);
    QList<GunMoveData> getGunMoveData(const TimeCondition *timeCondition=nullptr);
    QList<DeviceStatusInfo> getDeviceStatusInfos(const TimeCondition *timeCondition=nullptr);
    QList<AlarmInfo> getAlarmInfos(const TimeCondition *timeCondition=nullptr);
    QList<GunFiringData> getGunFiringData(const TimeCondition *timeCondition=nullptr);

    bool deleteData(int index, QString id);
private:
    bool isDbInited=false;
};

#endif // DBMANIPULATION_H
