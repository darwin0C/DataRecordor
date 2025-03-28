#include "devicestat.h"
#include "dboperator.h"

const int MaxDisLinkTime=25;
extern bool SDCardStatus;
DeviceStat::DeviceStat(int deviceId,QObject *parent) : QObject(parent)
{
    thisDeviceID=deviceId & 0xFF;
    mytimer=new QTimer();
    connect(mytimer,SIGNAL(timeout()),this,SLOT(timerStatHandle()));
    mytimer->start(1000);

    LinkCount=0;
    DeviceLinkStat=Device_Stat_DisLink;
    memset(&lastDeviceStat,0,sizeof (DeviceStatusInfo));
    lastDeviceStat.dateTime=TimeFormatTrans::getLongDataTime(QDateTime::currentDateTime());
    lastDeviceStat.deviceStatus.deviceAddress=thisDeviceID;
    qRegisterMetaType<DeviceStatusInfo>("DeviceStatusInfo");//自定义类型需要先注册

    deviceTotalWorkTime=DbOperator::Get()->getDeviceTotalWorkTimes(thisDeviceID);
}

bool DeviceStat::refreshStat(DeviceStatusInfo Stat)
{
    LinkCount=0;
    qDebug()<<"thisDeviceID:"<<QString::number(thisDeviceID,16);

    recoardWorkTime(TimeFormatTrans::getDateTime(Stat.dateTime));
    quint8 Status=lastDeviceStat.deviceStatus.Status ;// 设备状态
    memcpy(&lastDeviceStat,&Stat,sizeof (DeviceStatusInfo));
    if(Status!=Stat.deviceStatus.Status)
    {
        DeviceLinkStat=(Device_Stat)Stat.deviceStatus.Status;
        //memcpy(&lastDeviceStat,&Stat,sizeof (DeviceStatusInfo));
        return true;
    }
    return false;
}

void DeviceStat::timerStatHandle()
{
    if(thisDeviceID==0x89)//记录仪本身
    {
        //qDebug()<<"thisDeviceID"<<thisDeviceID<<"DeviceLinkStat "<<DeviceLinkStat;
        if(!SDCardStatus)
        {
            DeviceLinkStat=Device_Stat_Fault;
        }
        else
        {
            DeviceLinkStat=Device_Stat_Normal;
        }
        if(lastDeviceStat.deviceStatus.Status!=DeviceLinkStat)
        {
            lastDeviceStat.deviceStatus.Status=DeviceLinkStat;
            emit sig_StatChanged(lastDeviceStat);
        }
        return;
    }
    if(DeviceLinkStat!=Device_Stat_DisLink)
    {
        LinkCount++;
        if(LinkCount>MaxDisLinkTime)
        {
            DeviceLinkStat=Device_Stat_DisLink;
            lastDeviceStat.deviceStatus.Status=Device_Stat_DisLink;
            emit sig_StatChanged(lastDeviceStat);
        }
    }
    else
    {
        isWorking=false;
    }
}

void DeviceStat::recoardWorkTime(QDateTime endTime)
{
    if(!isWorking)
    {
        startTime=endTime;
        isWorking=true;
        return;
    }
    workTime+=startTime.secsTo(endTime);
    startTime=endTime;
    if(workTime>=60)
    {
        emit sig_WorkTimeRecord(lastDeviceStat.deviceStatus.deviceAddress);
        workTime=0;
        deviceTotalWorkTime.totalWorkTime++;
    }
}

//设备连接状态
int DeviceStat::LinkStat()
{
    return DeviceLinkStat;
}
DeviceStatusInfo DeviceStat::workStatus()
{
    return lastDeviceStat;
}
DeviceTotalWorkTime DeviceStat::deviceWorkTime()
{
    return deviceTotalWorkTime;
}
