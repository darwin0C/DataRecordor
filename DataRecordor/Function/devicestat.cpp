#include "devicestat.h"
#include "dboperator.h"

const int MaxDisLinkTime=5;

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

    qRegisterMetaType<DeviceStatusInfo>("DeviceStatusInfo");//自定义类型需要先注册

    deviceTotalWorkTime=DbOperator::Get()->getDeviceTotalWorkTimes(thisDeviceID);
}

bool DeviceStat::refreshStat(const DeviceStatusInfo &Stat)
{
    LinkCount=0;
    recoardWorkTime(TimeFormatTrans::getDateTime(Stat.dateTime));
    if(lastDeviceStat.deviceStatus.Status!=Stat.deviceStatus.Status)
    {
        DeviceLinkStat=(Device_Stat)Stat.deviceStatus.Status;
        memcpy(&lastDeviceStat,&Stat,sizeof (DeviceStatusInfo));
        //emit sig_StatChanged(lastDeviceStat);
        return true;
    }
    return false;
}

void DeviceStat::timerStatHandle()
{
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
