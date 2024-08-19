#include "devicestat.h"


const int MaxDisLinkTime=5;

DeviceStat::DeviceStat(QObject *parent) : QObject(parent)
{
    mytimer=new QTimer();
    connect(mytimer,SIGNAL(timeout()),this,SLOT(timerStatHandle()));
    mytimer->start(1000);

    LinkCount=0;
    DeviceLinkStat=Device_Stat_Init;
    memset(&lastDeviceStat,0,sizeof (DeviceStatusInfo));
    lastDeviceStat.dateTime=TimeFormatTrans::getLongDataTime(QDateTime::currentDateTime());

    qRegisterMetaType<DeviceStatusInfo>("DeviceStatusInfo");//自定义类型需要先注册
}

void DeviceStat::refreshStat(const DeviceStatusInfo &Stat)
{
    LinkCount=0;
    if(lastDeviceStat.deviceStatus.Status!=Stat.deviceStatus.Status)
    {
        DeviceLinkStat=(Device_Stat)Stat.deviceStatus.Status;
        memcpy(&lastDeviceStat,&Stat,sizeof (DeviceStatusInfo));
        emit sig_StatChanged(lastDeviceStat);
    }
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
}

//设备连接状态
int DeviceStat::LinkStat()
{
    return DeviceLinkStat;
}
