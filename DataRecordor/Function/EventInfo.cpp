#include "EventInfo.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

EventInfo::EventInfo(QObject *parent) : QObject(parent)
{
    QString deviceStatFile=QCoreApplication::applicationDirPath()+Event_CANDataFile;
    canReader.readCanDataFromXml(canDataList,deviceStatFile);
    qDebug()<<"canDataList"<<canDataList.count();
    getEventList();
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
    data.data[0]=0x0F;
    processCanData(data);
}
void EventInfo::processCanData(const CanData &data)
{
    if(!eventList.contains(data.dataid))
        return;
    QMap<QString,CanDataValue> dataMap= canReader.getValues(data,canDataList);
    qDebug()<<"dataMap"<<dataMap.count();
    refrushStat(data.dataid,dataMap,data.dateTime);
}

void EventInfo::getEventList()
{
    for(CanDataFormat canData:canDataList)
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
