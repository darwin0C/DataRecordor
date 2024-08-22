#include "dboperator.h"


DbOperator::DbOperator(QObject *parent) : QObject(parent)
{

}
///***************************************
///*插入数据
///*****************************************

// 插入函数实现
bool DbOperator::insertAlarmInfo(const AlarmInfo &alarmInfo) {
    return dbManipulation.insertAlarmInfo(alarmInfo);
}

bool DbOperator::insertGunMoveData(const GunMoveData &gunMoveData) {
    return dbManipulation.insertGunMoveData(gunMoveData);
}

bool DbOperator::insertGunFiringData(const GunFiringData &gunFiringData) {
    return dbManipulation.insertGunFiringData(gunFiringData);
}

bool DbOperator::insertDeviceName(const DeviceName &deviceName) {
    return dbManipulation.insertDeviceName(deviceName);
}

bool DbOperator::insertDeviceErrorInfo(const DeviceErrorInfo &errorInfo) {
    return dbManipulation.insertDeviceErrorInfo(errorInfo);
}

int DbOperator::insertDeviceStatusInfo(const DeviceStatusInfo &statusInfo) {
    return dbManipulation.insertDeviceStatusInfo(statusInfo);
}

bool DbOperator::insertDeviceTotalWorkTime(const DeviceTotalWorkTime &workTimeInfo) {
    return dbManipulation.insertDeviceTotalWorkTime(workTimeInfo);
}


///****************************************
///*更新数据
///******************************************/

// 更新函数实现
bool DbOperator::updateAlarmInfo(int id,const AlarmInfo &alarmInfo) {
    return dbManipulation.updateAlarmInfo(id,alarmInfo);
}

bool DbOperator::updateGunMoveData(int id,const GunMoveData &gunMoveData) {
    return dbManipulation.updateGunMoveData(id,gunMoveData);
}

bool DbOperator::updateGunFiringData(int id,const GunFiringData &gunFiringData) {
    return dbManipulation.updateGunFiringData(id,gunFiringData);
}

bool DbOperator::updateDeviceStatusInfo(const DeviceStatusInfo &statusInfo) {
    return dbManipulation.updateDeviceStatusInfo(statusInfo);
}

bool DbOperator::updateDeviceErrorInfo(const DeviceErrorInfo &errorInfo) {
    return dbManipulation.updateDeviceErrorInfo(errorInfo);
}

bool DbOperator::updateDeviceTotalWorkTime(const DeviceTotalWorkTime &workTimeInfo) {
    return dbManipulation.updateDeviceTotalWorkTime(workTimeInfo);
}

bool DbOperator::updateDeviceWorkTimeAdd1Minute(int deviceID) {
    return dbManipulation.updateDeviceTotalWorkTimeAdd1Minute(deviceID);
}


///****************************************
///*查询
///******************************************/

// 查询函数实现
QList<DeviceName> DbOperator::getDeviceNames() {
    return dbManipulation.getDeviceNames();
}

QList<DeviceErrorInfo> DbOperator::getDeviceErrorInfos(int statId) {
    return dbManipulation.getDeviceErrorInfos(statId);
}

DeviceTotalWorkTime DbOperator::getDeviceTotalWorkTimes(int deviceId) {
    return dbManipulation.getDeviceTotalWorkTimes(deviceId);
}

QList<GunMoveData> DbOperator::getGunMoveData(const TimeCondition *timeCondition) {
    return dbManipulation.getGunMoveData(timeCondition);
}

QList<DeviceStatusInfo> DbOperator::getDeviceStatusInfos(const TimeCondition *timeCondition) {
    return dbManipulation.getDeviceStatusInfos(timeCondition);
}

QList<AlarmInfo> DbOperator::getAlarmInfos(const TimeCondition *timeCondition) {
    return dbManipulation.getAlarmInfos(timeCondition);
}

QList<GunFiringData> DbOperator::getGunFiringData(const TimeCondition *timeCondition) {
    return dbManipulation.getGunFiringData(timeCondition);
}


///***************************************
///*删除
///*****************************************

// 删除函数实现
bool DbOperator::deleteData(int index, QString id) {
    return dbManipulation.deleteData(index, id);
}
