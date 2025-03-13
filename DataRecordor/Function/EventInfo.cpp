#include "EventInfo.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include "MsgSignals.h"
#include "qmycancomm.h"
#include "inisettings.h"
#include "commanager.h"
EventInfo::EventInfo(QObject *parent) : QObject(parent)
{
    QString deviceStatFile=QCoreApplication::applicationDirPath()+Event_CANDataFile;
    canReader.readCanDataFromXml(deviceStatFile);//读取CAN协议配置文件

    getEventList();
    connect(MsgSignals::getInstance(),&MsgSignals::canDataSig,this,&EventInfo::processCanData);
    connect(&alarmTimer,&QTimer::timeout,this,&EventInfo::alarmOntimeHandle);
    alarmTimer.start(1000*10);
    connect(QmyCanComm::instance(),&QmyCanComm::CanDataReady,this,&EventInfo::canLongDataHandle);

    memset(&gunAttitude,0,sizeof(GunAttitudeData));
    memset(&gunFiringData,0,sizeof(GunFiringData));
    //test();
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
    //0x0C8768CB
}
void EventInfo::processCanData(const CanData &data)
{
    deviceDataHandle(data);
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
void EventInfo::deviceDataHandle(const CanData &data)
{
    switch(data.dataid)
    {
    case  0x1CECFF68:    //导航数据	广播公告
        GunnerBroadcastRTSDataHandle(data.dataid,(uchar *)data.data);
        break;
    case  0x1CEBFF68:    //导航数据	广播数据
        GunnerBroadcastDTDataHandle((uchar *)data.data);
        break;
    case  0x1CEC8968:    //长包点对点的CTS
        GunnerP2PLongDataCTSData(data.dataid,(uchar *)data.data);
        break;
    case  0x1CEB8968:    //长包数据
        QmyCanComm::instance()->recieveTM_DT((uchar *)data.data);

        break;
    }
}
void EventInfo::resbanceToGunner(uchar *buff,int len)
{
    uint canID=0x0CE36889;
    ComManager::instance()->sendRecordCanData(canID,buff,len);
}
//接受惯导广播公告数据
void EventInfo::GunnerBroadcastRTSDataHandle(unsigned int canID,uchar *buff)
{
    QmyCanComm::instance()->recieveLinkDataHandle(canID,buff);
}
//接受惯导广播数据处理
void EventInfo::GunnerBroadcastDTDataHandle(uchar *buff)
{
    QmyCanComm::instance()->recieveBroadcastTM_DT(buff);
}
//接收长数据包点对点的CTS
void EventInfo::GunnerP2PLongDataCTSData(uint canId,uchar *buff)
{
    QmyCanComm::instance()->recieveLinkDataHandle(canId,buff);
}

void EventInfo::canLongDataHandle(unsigned char *png ,unsigned char *buff ,unsigned short len ,bool bbroadcast)
{
    //if(bbroadcast)
    {
        uint tem=png[0]|png[1]<<8|png[2]<<16;
        SelfAttributeData selfAttributeData;
        if(tem==Gunner_Property_PGN && len==sizeof(SelfAttributeData))
        {
            if(memcmp(&selfAttributeData,buff,sizeof(SelfAttributeData))!=0)
            {
                memcpy(&selfAttributeData,buff,sizeof(SelfAttributeData));
            }
            //emit sig_StatChanged(INUData_Navigate);
            qDebug()<<"canLongDataHandle"<<selfAttributeData.attribute<<selfAttributeData.uniqueID;
            iniSettings::Instance()->setAttribute(selfAttributeData.attribute,selfAttributeData.uniqueID);
            uchar a=0x0F;
            resbanceToGunner(&a,1);
        }
    }
}
void EventInfo::refrushStat(int dataId,const QMap<QString,CanDataValue> &dataMap,LocalDateTime dateTime )
{
    QString sigName=eventList[dataId];
    if(sigName=="GunBarrelDirection")
    {
        gunMoveData.barrelDirection=dataMap["BarrelDirection"].value.toUInt();
        gunMoveData.elevationAngle=dataMap["ElevationAngle"].value.toUInt();

        gunAttitude.barrelDirection=dataMap["BarrelDirection"].value.toUInt();
        gunAttitude.elevationAngle=dataMap["ElevationAngle"].value.toUInt();
    }
    else if(sigName=="SensorData")
    {
        gunMoveData.chassisRoll=dataMap["Roll"].value.toUInt();
        gunMoveData.chassisPitch=dataMap["Pitch"].value.toUInt();

        gunAttitude.chassisRoll=dataMap["Roll"].value.toUInt();
        gunAttitude.chassisPitch=dataMap["Pitch"].value.toUInt();
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
        isRevShootSignal=true;

        gunFiringData.attitudeData=gunAttitude;
        gunFiringData.firingCompletedSignal=dataMap["ShootDoneSignal"].value.toUInt();
        gunFiringData.recoilStatus=dataMap["RecoilStatus"].value.toUInt();
        gunFiringData.statusChangeTime=TimeFormatTrans::convertToLongDateTime(dateTime);
        gunFiringData.propellantTemperature=propellantTemperature;
        // 创建一个单次执行的定时器
        QTimer::singleShot(2000,this, &EventInfo::onTimeout);
    }
    else if(sigName=="GunpowderTemp")
    {
        propellantTemperature=dataMap["GunpowderTemp"].value.toUInt();
        //gunFiringData.propellantTemperature=dataMap["GunpowderTemp"].value.toUInt();
    }
    else if(sigName=="InitSpeed")
    {
        isInitSpeedreceived=true;
        gunFiringData.muzzleVelocityValid=1;

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
    //接收到初速信息，主动上报射击数据
    if(isInitSpeedreceived)
    {
        saveGunFiringData();
        if(isAutoSendEnabled)
        {
            SendGunFiringData(gunFiringData);//发送到炮长终端
            //发送到指挥终端
            emit sendCommandDataSig(DataFlag_GunshootData, QByteArray(reinterpret_cast<const char*>(&gunFiringData), sizeof(GunFiringData)));
        }
        isInitSpeedreceived=false;
    }
}
//炮长获取射击数据
int EventInfo::SendGunFiringData(GunFiringData gunfirngData)
{
    //长数据包发送先发送RTS
    unsigned char pgn[3];
    pgn[0]=0x03;
    pgn[1]=0xFF;
    pgn[2]=0x0;
    //计算发送报数
    unsigned char packets=0;//包数
    unsigned short b=sizeof(GunFiringData);
    if(b%7==0)
        packets=b/7;
    else
        packets=b/7+1;
    qDebug()<<"SendGunFiringData"<<packets;
    int a= QmyCanComm::instance()->sendTM_RTS(Gunner_Ctrol_LANG_ID,b,packets,pgn,(unsigned char*)&gunfirngData);
    qDebug()<< "GunFiringData("
            << "barrelDirection: " << gunfirngData.attitudeData.barrelDirection << ", "
            << "elevationAngle: " << gunfirngData.attitudeData.elevationAngle << ", "
            << "chassisRoll: " << gunfirngData.attitudeData.chassisRoll << ", "
            << "chassisPitch: " << gunfirngData.attitudeData.chassisPitch << ", "
            << "statusChangeTime: "
            << gunfirngData.statusChangeTime.ti_year
            << gunfirngData.statusChangeTime.ti_mon
            << gunfirngData.statusChangeTime.ti_day
            << gunfirngData.statusChangeTime.ti_hour
            << gunfirngData.statusChangeTime.ti_min
            << gunfirngData.statusChangeTime.ti_sec << ", "
            << "firingCompletedSignal: " << static_cast<int>(gunfirngData.firingCompletedSignal) << ", "
            << "recoilStatus: " << static_cast<int>(gunfirngData.recoilStatus) << ", "
            << "muzzleVelocityValid: " << static_cast<int>(gunfirngData.muzzleVelocityValid) << ", "
            << "propellantTemperature: " << gunfirngData.propellantTemperature << ", "
            << "muzzleVelocity: " << gunfirngData.muzzleVelocity
            << ")";
    return a;
}
//定时上报报警状态
void EventInfo::alarmOntimeHandle()
{
    nuclearBioAlarmCount++;
    fireSuppressAlarmCount++;
#ifdef TEST_MODE
    //SendGunFiringData(gunFiringData);//发送到炮长终端
#endif
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
