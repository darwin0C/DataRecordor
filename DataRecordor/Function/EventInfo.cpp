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
        AlarmInfo alarmInfo;
        alarmInfo.alarmContent=0;
        alarmInfo.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        alarmInfo.alarmDetails=dataMap["Alarm"].value.toUInt()& 0xFFFF;
        saveAlarmInfo(alarmInfo);
    }
    else if(sigName=="FireExtinguishSuppressAlarm")//灭火抑爆报警
    {
        AlarmInfo alarmInfo;
        alarmInfo.alarmContent=1;
        alarmInfo.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        alarmInfo.alarmDetails=dataMap["Alarm"].value.toUInt() & 0xFF;
        saveAlarmInfo(alarmInfo);
    }
}

void EventInfo::onTimeout()
{
    if(isInitSpeedreceived)
    {
        saveGunFiringData();
        isInitSpeedreceived=false;
    }
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
