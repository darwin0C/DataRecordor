<<<<<<< HEAD
#include "EventInfo.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include "MsgSignals.h"
EventInfo::EventInfo(QObject *parent) : QObject(parent)
{
    QString deviceStatFile=QCoreApplication::applicationDirPath()+Event_CANDataFile;
    canReader.readCanDataFromXml(deviceStatFile);//读取CAN协议配置文件

    getEventList();
    connect(MsgSignals::getInstance(),&MsgSignals::canDataSig,this,&EventInfo::processCanData);
    connect(&alarmTimer,&QTimer::timeout,this,&EventInfo::alarmOntimeHandle);
    alarmTimer.start(2000);
    test();
}
void EventInfo::test()
{
    CanData data;
    memset(&data,0xFF,sizeof(CanData));
    data.dateTime={2024,8,20,11,29,0,0};
    data.len=8;
    data.dataid=0x0CF220A8;
    data.data[0]=0x0F;
    processCanData(data);
    qDebug()<<"11";
    data.dataid=0x0C8768CB;
    processCanData(data);
    data.dataid=0x0CF3A229;
    processCanData(data);
    data.dataid=0x0CF08534;
    processCanData(data);
    data.dataid=0x0CF08634;
    processCanData(data);
}
void EventInfo::processCanData(const CanData &data)
{
    if(!eventList.contains(data.dataid))
        return;
    QMap<QString,CanDataValue> dataMap= canReader.getValues(data);
    qDebug()<<"dataMap"<<dataMap.count();
    refrushStat(data.dataid,dataMap,data.dateTime);
}

void EventInfo::getEventList()
{
    for(const CanDataFormat &canData:canReader.getCanDataList())
    {
        eventList[canData.id]=canData.msgName;
    }
}

void EventInfo::refrushStat(int dataId,const QMap<QString,CanDataValue> &dataMap,LocalDateTime dateTime )
{
    QString sigName=eventList[dataId];
    if(sigName=="GunBarrelDirection")
    {
        gunMoveData.barrelDirection=dataMap["BarrelDirection"].value.toUInt();
        gunMoveData.elevationAngle=dataMap["ElevationAngle"].value.toUInt();

        gunFiringData.barrelDirection=dataMap["BarrelDirection"].value.toUInt();
        gunFiringData.elevationAngle=dataMap["ElevationAngle"].value.toUInt();
    }
    else if(sigName=="SensorData")
    {
        gunMoveData.chassisRoll=dataMap["Roll"].value.toUInt();
        gunMoveData.chassisPitch=dataMap["Pitch"].value.toUInt();

        gunFiringData.chassisRoll=dataMap["Roll"].value.toUInt();
        gunFiringData.chassisPitch=dataMap["Pitch"].value.toUInt();
    }
    else if(sigName=="AutoGunMove")
    {
        gunMoveData.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        gunMoveData.autoAdjustmentStatus=dataMap["GunMoveStat"].value.toUInt();
        saveGunMovedata();
        if(isAutoSendEnabled)
            emit sendCommandDataSig(DataFlag_GunMoveData, QByteArray(reinterpret_cast<const char*>(&gunMoveData), sizeof(GunMoveData)));

    }
    else if(sigName=="ShootDone")
    {
        gunFiringData.firingCompletedSignal=dataMap["ShootDoneSignal"].value.toUInt();
        gunFiringData.recoilStatus=dataMap["RecoilStatus"].value.toUInt();
        gunFiringData.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        // 创建一个单次执行的定时器
        QTimer::singleShot(2000,this, &EventInfo::onTimeout);
    }
    else if(sigName=="GunpowderTemp")
    {
        gunFiringData.propellantTemperature=dataMap["GunpowderTemp"].value.toUInt();
    }
    else if(sigName=="InitSpeed")
    {
        isInitSpeedreceived=true;
        gunFiringData.muzzleVelocity=dataMap["InitSpeed"].value.toUInt();
    }
    else if(sigName=="NuclearBioAlarm")//核生化报警
    {
        nuclearBioAlarmCount=0;

        nuclearBioAlarmInfo.alarmContent=0;
        nuclearBioAlarmInfo.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        nuclearBioAlarmInfo.alarmDetails=dataMap["Alarm"].value.toUInt()& 0xFFFF;
        saveAlarmInfo(nuclearBioAlarmInfo);
    }
    else if(sigName=="FireExtinguishSuppressAlarm")//灭火抑爆报警
    {
        fireSuppressAlarmCount=0;

        fireSuppressBioAlarmInfo.alarmContent=1;
        fireSuppressBioAlarmInfo.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        fireSuppressBioAlarmInfo.alarmDetails=dataMap["Alarm"].value.toUInt() & 0xFF;
        saveAlarmInfo(fireSuppressBioAlarmInfo);
    }
}

void EventInfo::setAutoReport(bool enable)
{
    isAutoSendEnabled=enable;
    //    if(isAutoSendEnabled)
    //        alarmTimer.start(2000);
    //    else
    //        alarmTimer.stop();
}

void EventInfo::onTimeout()
{
    if(isInitSpeedreceived)
    {
        saveGunFiringData();
        if(isAutoSendEnabled)
            emit sendCommandDataSig(DataFlag_GunshootData, QByteArray(reinterpret_cast<const char*>(&gunFiringData), sizeof(GunFiringData)));
        isInitSpeedreceived=false;
    }
}
//定时上报报警状态
void EventInfo::alarmOntimeHandle()
{
    nuclearBioAlarmCount++;
    fireSuppressAlarmCount++;

    QByteArray dataArray=getCurrentAlarmData();
    if(!dataArray.isEmpty() && isAutoSendEnabled)
        emit sendCommandDataSig(DataFlag_AlarmInfo, dataArray);
}

QByteArray EventInfo::getCurrentAlarmData()
{
    QByteArray dataArray;
    quint8 alarmCount=0;
    if(nuclearBioAlarmCount<5)
    {
        alarmCount++;
        dataArray.append(QByteArray(reinterpret_cast<const char*>(&nuclearBioAlarmInfo), sizeof(AlarmInfo)));
    }
    if(fireSuppressAlarmCount<5)
    {
        alarmCount++;
        dataArray.append(QByteArray(reinterpret_cast<const char*>(&fireSuppressBioAlarmInfo), sizeof(AlarmInfo)));
    }
    if(alarmCount>0)
    {
        dataArray.prepend(alarmCount);
    }
    return dataArray;
}


void EventInfo::saveGunMovedata()
{
    DbOperator::Get()->insertGunMoveData(gunMoveData);
}
void EventInfo::saveGunFiringData()
{
    DbOperator::Get()->insertGunFiringData(gunFiringData);
}

void EventInfo::saveAlarmInfo(const AlarmInfo &alarmInfo)
{
    DbOperator::Get()->insertAlarmInfo(alarmInfo);
}

GunMoveData EventInfo::getGunMoveData()
{
    return gunMoveData;
}

GunFiringData EventInfo::getGunFiringData()
{
    return gunFiringData;
}


//获取历史调炮数据
QList<GunMoveData> EventInfo::getHistoryGunMoveData(TimeCondition *timeConditionPtr)
{
    return DbOperator::Get()->getGunMoveData(timeConditionPtr);
}

//获取历史射击数据
QList<GunFiringData> EventInfo::getHistoryGunFiringData(TimeCondition *timeConditionPtr)
{
    return DbOperator::Get()->getGunFiringData(timeConditionPtr);
}
=======
#include "EventInfo.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include "MsgSignals.h"
EventInfo::EventInfo(QObject *parent) : QObject(parent)
{
    QString deviceStatFile=QCoreApplication::applicationDirPath()+Event_CANDataFile;
    canReader.readCanDataFromXml(deviceStatFile);//读取CAN协议配置文件

    getEventList();
    connect(MsgSignals::getInstance(),&MsgSignals::canDataSig,this,&EventInfo::processCanData);
    connect(&alarmTimer,&QTimer::timeout,this,&EventInfo::alarmOntimeHandle);
    alarmTimer.start(2000);
    test();
}
void EventInfo::test()
{
    CanData data;
    memset(&data,0xFF,sizeof(CanData));
    data.dateTime={2024,8,20,11,29,0,0};
    data.len=8;
    data.dataid=0x0CF220A8;
    data.data[0]=0x0F;
    processCanData(data);
    qDebug()<<"11";
    data.dataid=0x0C8768CB;
    processCanData(data);
    data.dataid=0x0CF3A229;
    processCanData(data);
    data.dataid=0x0CF08534;
    processCanData(data);
    data.dataid=0x0CF08634;
    processCanData(data);
}
void EventInfo::processCanData(const CanData &data)
{
    if(!eventList.contains(data.dataid))
        return;
    QMap<QString,CanDataValue> dataMap= canReader.getValues(data);
    qDebug()<<"dataMap"<<dataMap.count();
    refrushStat(data.dataid,dataMap,data.dateTime);
}

void EventInfo::getEventList()
{
    for(const CanDataFormat &canData:canReader.getCanDataList())
    {
        eventList[canData.id]=canData.msgName;
    }
}

void EventInfo::refrushStat(int dataId,const QMap<QString,CanDataValue> &dataMap,LocalDateTime dateTime )
{
    QString sigName=eventList[dataId];
    if(sigName=="GunBarrelDirection")
    {
        gunMoveData.barrelDirection=dataMap["BarrelDirection"].value.toUInt();
        gunMoveData.elevationAngle=dataMap["ElevationAngle"].value.toUInt();

        gunFiringData.barrelDirection=dataMap["BarrelDirection"].value.toUInt();
        gunFiringData.elevationAngle=dataMap["ElevationAngle"].value.toUInt();
    }
    else if(sigName=="SensorData")
    {
        gunMoveData.chassisRoll=dataMap["Roll"].value.toUInt();
        gunMoveData.chassisPitch=dataMap["Pitch"].value.toUInt();

        gunFiringData.chassisRoll=dataMap["Roll"].value.toUInt();
        gunFiringData.chassisPitch=dataMap["Pitch"].value.toUInt();
    }
    else if(sigName=="AutoGunMove")
    {
        gunMoveData.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        gunMoveData.autoAdjustmentStatus=dataMap["GunMoveStat"].value.toUInt();
        saveGunMovedata();
        if(isAutoSendEnabled)
            emit sendCommandDataSig(DataFlag_GunMoveData, QByteArray(reinterpret_cast<const char*>(&gunMoveData), sizeof(GunMoveData)));

    }
    else if(sigName=="ShootDone")
    {
        gunFiringData.firingCompletedSignal=dataMap["ShootDoneSignal"].value.toUInt();
        gunFiringData.recoilStatus=dataMap["RecoilStatus"].value.toUInt();
        gunFiringData.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        // 创建一个单次执行的定时器
        QTimer::singleShot(2000,this, &EventInfo::onTimeout);
    }
    else if(sigName=="GunpowderTemp")
    {
        gunFiringData.propellantTemperature=dataMap["GunpowderTemp"].value.toUInt();
    }
    else if(sigName=="InitSpeed")
    {
        isInitSpeedreceived=true;
        gunFiringData.muzzleVelocity=dataMap["InitSpeed"].value.toUInt();
    }
    else if(sigName=="NuclearBioAlarm")//核生化报警
    {
        nuclearBioAlarmCount=0;

        nuclearBioAlarmInfo.alarmContent=0;
        nuclearBioAlarmInfo.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        nuclearBioAlarmInfo.alarmDetails=dataMap["Alarm"].value.toUInt()& 0xFFFF;
        saveAlarmInfo(nuclearBioAlarmInfo);
    }
    else if(sigName=="FireExtinguishSuppressAlarm")//灭火抑爆报警
    {
        fireSuppressAlarmCount=0;

        fireSuppressBioAlarmInfo.alarmContent=1;
        fireSuppressBioAlarmInfo.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        fireSuppressBioAlarmInfo.alarmDetails=dataMap["Alarm"].value.toUInt() & 0xFF;
        saveAlarmInfo(fireSuppressBioAlarmInfo);
    }
}

void EventInfo::setAutoReport(bool enable)
{
    isAutoSendEnabled=enable;
    //    if(isAutoSendEnabled)
    //        alarmTimer.start(2000);
    //    else
    //        alarmTimer.stop();
}

void EventInfo::onTimeout()
{
    if(isInitSpeedreceived)
    {
        saveGunFiringData();
        if(isAutoSendEnabled)
            emit sendCommandDataSig(DataFlag_GunshootData, QByteArray(reinterpret_cast<const char*>(&gunFiringData), sizeof(GunFiringData)));
        isInitSpeedreceived=false;
    }
}
//定时上报报警状态
void EventInfo::alarmOntimeHandle()
{
    nuclearBioAlarmCount++;
    fireSuppressAlarmCount++;

    QByteArray dataArray=getCurrentAlarmData();
    if(!dataArray.isEmpty() && isAutoSendEnabled)
        emit sendCommandDataSig(DataFlag_AlarmInfo, dataArray);
}

QByteArray EventInfo::getCurrentAlarmData()
{
    QByteArray dataArray;
    quint8 alarmCount=0;
    if(nuclearBioAlarmCount<5)
    {
        alarmCount++;
        dataArray.append(QByteArray(reinterpret_cast<const char*>(&nuclearBioAlarmInfo), sizeof(AlarmInfo)));
    }
    if(fireSuppressAlarmCount<5)
    {
        alarmCount++;
        dataArray.append(QByteArray(reinterpret_cast<const char*>(&fireSuppressBioAlarmInfo), sizeof(AlarmInfo)));
    }
    if(alarmCount>0)
    {
        dataArray.prepend(alarmCount);
    }
    return dataArray;
}


void EventInfo::saveGunMovedata()
{
    DbOperator::Get()->insertGunMoveData(gunMoveData);
}
void EventInfo::saveGunFiringData()
{
    DbOperator::Get()->insertGunFiringData(gunFiringData);
}

void EventInfo::saveAlarmInfo(const AlarmInfo &alarmInfo)
{
    DbOperator::Get()->insertAlarmInfo(alarmInfo);
}

GunMoveData EventInfo::getGunMoveData()
{
    return gunMoveData;
}

GunFiringData EventInfo::getGunFiringData()
{
    return gunFiringData;
}


//获取历史调炮数据
QList<GunMoveData> EventInfo::getHistoryGunMoveData(TimeCondition *timeConditionPtr)
{
    return DbOperator::Get()->getGunMoveData(timeConditionPtr);
}

//获取历史射击数据
QList<GunFiringData> EventInfo::getHistoryGunFiringData(TimeCondition *timeConditionPtr)
{
    return DbOperator::Get()->getGunFiringData(timeConditionPtr);
}
>>>>>>> a47cf4023014f66bb831bba7d44fd896e866a872
