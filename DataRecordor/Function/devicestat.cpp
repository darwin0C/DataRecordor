#include "devicestat.h"

#include <QDebug>
DeviceStat::DeviceStat(QObject *parent) : QObject(parent)
{

    canReader.readCanDataFromXml(canDataList,"D:/CAN.xml");
    qDebug()<<"canDataList"<<canDataList.count();
    getDeviceLists();
    CanData data;
    memset(&data,0xFF,sizeof(CanData));
    data.len=8;
    data.dataid=0x0CF1A148;
    data.data[0]=0x0F;
    processCanData(data);
    qDebug()<<"11";
}


void DeviceStat::processCanData(const CanData &data)
{
    if(!deviceList.contains(data.dataid))
        return;
    QMap<QString,CanDataValue> dataMap= canReader.getValues(data,canDataList);
    qDebug()<<"dataMap"<<dataMap.count();
}

void DeviceStat::getDeviceLists()
{
    for(const CanDataFormat &device:canDataList)
    {
        deviceList.append(device.id);
    }
    qDebug()<<"deviceList"<<deviceList.count();
}
