#include "devicestat.h"
#include "canmsgreader.h"

DeviceStat::DeviceStat(QObject *parent) : QObject(parent)
{
    QList<CanDataFormat> canDataList;
    CanMsgReader canReader;
    canReader.readCanDataFromXml(canDataList,"D:/can.xml");
}
