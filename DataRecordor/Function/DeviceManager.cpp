#include "DeviceManager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>
DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    QString deviceStatFile=QCoreApplication::applicationDirPath()+DeviceStat_CANDataFile;
    canReader.readCanDataFromXml(canDataList,deviceStatFile);
    qDebug()<<"canDataList"<<canDataList.count();
    getDeviceLists();

    dbOperator=DbOperator::Get();
    QString deviceNameFile=QCoreApplication::applicationDirPath()+DeviceNameFile;
    //loadAndInsertDevicesFromXml(deviceNameFile);
}

void DeviceManager::test()
{
    CanData data;
    memset(&data,0xFF,sizeof(CanData));
    data.len=8;
    data.dataid=0x0CF1A148;
    data.data[0]=0x0F;
    processCanData(data);
    qDebug()<<"11";
}

void DeviceManager::processCanData(const CanData &data)
{
    if(!deviceList.contains(data.dataid))
        return;
    QMap<QString,CanDataValue> dataMap= canReader.getValues(data,canDataList);
    qDebug()<<"dataMap"<<dataMap.count();
    saveDeviceStat(dataMap);
}

void DeviceManager::saveDeviceStat(const QMap<QString,CanDataValue> &dataMap)
{
    for(auto it=dataMap.begin();it!=dataMap.end();++it)
    {
        if(it.key()=="WorkStatus")
        {
        }
    }
}



void DeviceManager::getDeviceLists()
{
    for(const CanDataFormat &device:canDataList)
    {
        deviceList.append(device.id);
    }
    qDebug()<<"deviceList"<<deviceList.count();
}

bool DeviceManager::loadAndInsertDevicesFromXml(const QString &xmlFilePath) {
    QFile file(xmlFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << xmlFilePath;
        return false;
    }

    QXmlStreamReader xmlReader(&file);
    while (!xmlReader.atEnd()) {
        xmlReader.readNext();

        if (xmlReader.hasError()) {
            qDebug() << "XML parsing error:" << xmlReader.errorString();
            file.close();
            return false;
        }

        if (xmlReader.isStartElement()) {
            qDebug() <<xmlReader.name();
            if (xmlReader.name() == "Device") {
                DeviceName device;
                QString addressStr = xmlReader.attributes().value("Address").toString();

                if (addressStr.isEmpty()) {
                    qDebug() << "Address attribute missing for device";
                    continue;
                }

                // 读取设备地址并转换为十进制数
                device.deviceAddress = addressStr.toInt(nullptr, 16);

                // 读取设备名称
                device.deviceName = xmlReader.attributes().value("Name").toString();

                if (device.deviceName.isEmpty()) {
                    qDebug() << "Name attribute missing for device";
                    continue;
                }

                // 插入设备信息到数据库
                if (!dbOperator->insertDeviceName(device)) {
                    qDebug() << "Failed to insert device:" << device.deviceName;
                } else {
                    qDebug() << "Inserted device:" << device.deviceName << "with address" << addressStr;
                }
            }
        }
    }

    file.close();
    return true;
}
