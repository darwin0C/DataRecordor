#ifndef DBOPERATOR_H
#define DBOPERATOR_H

#include <QObject>
#include "dbDatas.h"
#include "dbmanipulation.h"


class DbOperator : public QObject
{
    Q_OBJECT
    DbManipulation dbManipulation;
public:
    explicit DbOperator(QObject *parent = nullptr) ;

    static DbOperator *Get()
    {
        static DbOperator vt;
        return &vt;
    }
    // 删除拷贝构造函数和赋值操作符，防止对象拷贝
    DbOperator(const DbOperator&) = delete;
    DbOperator& operator=(const DbOperator&) = delete;

    bool DeleteEquStatData(QString id) ;

    bool insertAlarmInfo(const AlarmInfo &alarmInfo);
    bool insertGunMoveData(const GunMoveData &gunMoveData);
    bool insertGunFiringData(const GunFiringData &gunFiringData);
    bool insertDeviceName(const DeviceName &deviceName);
    bool insertDeviceErrorInfo(const DeviceErrorInfo &errorInfo);
    int insertDeviceStatusInfo(const DeviceStatusInfo &statusInfo);
    bool insertDeviceTotalWorkTime(const DeviceTotalWorkTime &workTimeInfo);

    bool updateAlarmInfo(int id,const AlarmInfo &alarmInfo);
    bool updateGunMoveData(int id,const GunMoveData &gunMoveData);
    bool updateGunFiringData(int id,const GunFiringData &gunFiringData);
    bool updateDeviceStatusInfo(const DeviceStatusInfo &statusInfo);
    bool updateDeviceErrorInfo(const DeviceErrorInfo &errorInfo);
    bool updateDeviceTotalWorkTime(const DeviceTotalWorkTime &workTimeInfo);
    bool updateDeviceWorkTimeAdd1Minute(int deviceID);

    QList<DeviceName> getDeviceNames();
    QList<DeviceErrorInfo> getDeviceErrorInfos(int statId);
    DeviceTotalWorkTime getDeviceTotalWorkTimes(int deviceId);
    QList<GunMoveData> getGunMoveData(const TimeCondition *timeCondition);
    QList<DeviceStatusInfo> getDeviceStatusInfos(const TimeCondition *timeCondition);
    QList<AlarmInfo> getAlarmInfos(const TimeCondition *timeCondition);
    QList<GunFiringData> getGunFiringData(const TimeCondition *timeCondition);

    bool deleteData(int index, QString id);

signals:

};

#endif // DBOPERATOR_H
