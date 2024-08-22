<<<<<<< HEAD
#include "DeviceManager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>
#include "MsgSignals.h"
DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    QString deviceStatFile=QCoreApplication::applicationDirPath()+DeviceStat_CANDataFile;
    canReader.readCanDataFromXml(deviceStatFile);//读取CAN协议配置文件

    getDeviceLists();

    dbOperator=DbOperator::Get();
    QString deviceNameFile=QCoreApplication::applicationDirPath()+DeviceNameFile;
    loadAndInsertDevicesFromXml(deviceNameFile);

    connect(MsgSignals::getInstance(),&MsgSignals::canDataSig,this,&DeviceManager::processCanData);

    test();
}

void DeviceManager::test()
{
    CanData data;
    memset(&data,0xFF,sizeof(CanData));
    data.dateTime={2024,8,20,11,29,0,0};
    data.len=8;
    data.dataid=0x0CF1A148;
    data.data[0]=0x0F;
    processCanData(data);
    qDebug()<<"11";
}

void DeviceManager::processCanData(const CanData &data)
{
    if(!devices.contains(data.dataid))
        return;
    QMap<QString,CanDataValue> dataMap= canReader.getValues(data);
    qDebug()<<"dataMap"<<dataMap.count();

    DeviceStatusInfo deviceIndo;
    deviceIndo.dateTime=TimeFormatTrans::convertToLongDateTime(data.dateTime);
    deviceIndo.deviceStatus.Status=dataMap["WorkStatus"].value.toUInt();
    deviceIndo.deviceStatus.deviceAddress=data.dataid & 0xFF;

    memcpy( &deviceIndo.deviceStatus.faultInfo,data.data+1,7);

    //判断状态是否变化
    if(devices[data.dataid]->refreshStat(deviceIndo))
    {
        int statId= saveStat(deviceIndo);
        if(statId>0)
            saveDeviceStatSignals(statId,dataMap);
    }
}

//保存到数据库
int DeviceManager::saveStat(const DeviceStatusInfo &deviceIndo)
{
    return   dbOperator->insertDeviceStatusInfo(deviceIndo);
}

void DeviceManager::saveDeviceStatSignals(int statID,const QMap<QString,CanDataValue> &dataMap)
{
    for(auto it=dataMap.begin();it!=dataMap.end();++it)
    {

        DeviceErrorInfo errorInfo;
        errorInfo.statId=statID;
        errorInfo.errorInfo=QString("%1: %2 %3").arg(it.key()).arg(it.value().value.toInt()).arg(it.value().valueDescription);
        dbOperator->insertDeviceErrorInfo(errorInfo);

        qDebug()<<it.key()<<it.value().value.toInt()<<it.value().valueDescription;
    }
}

//获取设备列表
void DeviceManager::getDeviceLists()
{
    for(const CanDataFormat &device:canReader.getCanDataList())
    {
        DeviceStat *deviceStat=new DeviceStat(device.id,this);
        connect(deviceStat,&DeviceStat::sig_WorkTimeRecord,this,&DeviceManager::recordWorkTime);
        connect(deviceStat,&DeviceStat::sig_StatChanged,this,&DeviceManager::StatWorkChange);
        devices[device.id]=deviceStat;
    }
    qDebug()<<"deviceList"<<devices.count();
}

void DeviceManager::recordWorkTime(int deviceID)
{
    dbOperator->updateDeviceWorkTimeAdd1Minute(deviceID);
}

void DeviceManager::StatWorkChange(DeviceStatusInfo deviceInfo)
{
    saveStat(deviceInfo);
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
                DeviceTotalWorkTime workTimeInfo;
                workTimeInfo.deviceId=device.deviceAddress;
                workTimeInfo.totalWorkTime=0;
                // 插入设备工作时间到数据库
                if (!dbOperator->insertDeviceTotalWorkTime(workTimeInfo)) {
                    qDebug() << "Failed to insert device worktime:" << device.deviceName;
                } else {
                    qDebug() << "Inserted device worktime:" << device.deviceName << "with address" << addressStr;
                }
            }
        }
    }
    file.close();
    return true;
}

//获取当前故障设备信息
QByteArray DeviceManager::getErrorDeviceStat(int &deviceCount)
{
    QByteArray array;
    deviceCount = 0;
    for(auto it = devices.begin(); it != devices.end(); ++it)
    {
        if(it.value() != nullptr) // 检查指针合法性
        {
            DeviceStatus deviceStatus = it.value()->workStatus().deviceStatus;
            if(deviceStatus.Status!=0x0F)
            {
                array.append(QByteArray(reinterpret_cast<const char*>(&deviceStatus), sizeof(DeviceStatus)));
                deviceCount++;
            }
        }
    }
    return array;
}

//根据设备地址获取设备信息
QByteArray DeviceManager::getDeviceStat(int deviceAddress, int &deviceCount)
{
    QByteArray array;
    deviceCount = 0;

    for(auto it = devices.begin(); it != devices.end(); ++it)
    {
        if(it.value() != nullptr) // 检查指针合法性
        {
            if(deviceAddress == 0 || (it.key() & 0xFF) == deviceAddress) // 优先级修正
            {
                DeviceStatus deviceStatus = it.value()->workStatus().deviceStatus;
                array.append(QByteArray(reinterpret_cast<const char*>(&deviceStatus), sizeof(DeviceStatus)));
                deviceCount++;
                if(deviceAddress != 0) // 如果找到指定设备，直接返回
                {
                    break;
                }
            }
        }
    }
    return array;
}

//根据设备地址获取设备故障信息
QList<DeviceStatusInfo> DeviceManager::getHistoryDeviceStat(int deviceAddress,TimeCondition *timeConditionPtr)
{
    QList<DeviceStatusInfo> statInfoList;
    QList<DeviceStatusInfo> statInfoListTemp= dbOperator->getDeviceStatusInfos(timeConditionPtr);
    for(const DeviceStatusInfo &statInfo:statInfoListTemp)
    {
        if(deviceAddress==0 || deviceAddress==statInfo.deviceStatus.deviceAddress)
        {
            statInfoList.append(statInfo);
        }

    }
    return statInfoList;
}


//根据设备地址获取设备累计工作时间
QByteArray DeviceManager::getDeviceTotalWorktime(int deviceAddress, int &deviceCount)
{
    QByteArray array;
    deviceCount = 0;

    for(auto it = devices.begin(); it != devices.end(); ++it)
    {
        if(it.value() != nullptr) // 检查指针合法性
        {
            if(deviceAddress == 0 || (it.key() & 0xFF) == deviceAddress) // 优先级修正
            {
                DeviceTotalWorkTime deviceWorkTime = it.value()->deviceWorkTime();
                array.append(QByteArray(reinterpret_cast<const char*>(&deviceWorkTime), sizeof(DeviceTotalWorkTime)));
                deviceCount++;
                if(deviceAddress != 0) // 如果找到指定设备，直接返回
                {
                    break;
                }
            }
        }
    }
    return array;
}
=======
#include "DeviceManager.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>
#include "MsgSignals.h"
DeviceManager::DeviceManager(QObject *parent) : QObject(parent)
{
    QString deviceStatFile=QCoreApplication::applicationDirPath()+DeviceStat_CANDataFile;
    canReader.readCanDataFromXml(deviceStatFile);//读取CAN协议配置文件

    getDeviceLists();

    dbOperator=DbOperator::Get();
    QString deviceNameFile=QCoreApplication::applicationDirPath()+DeviceNameFile;
    loadAndInsertDevicesFromXml(deviceNameFile);

    connect(MsgSignals::getInstance(),&MsgSignals::canDataSig,this,&DeviceManager::processCanData);

    test();
}

void DeviceManager::test()
{
    CanData data;
    memset(&data,0xFF,sizeof(CanData));
    data.dateTime={2024,8,20,11,29,0,0};
    data.len=8;
    data.dataid=0x0CF1A148;
    data.data[0]=0x0F;
    processCanData(data);
    qDebug()<<"11";
}

void DeviceManager::processCanData(const CanData &data)
{
    if(!devices.contains(data.dataid))
        return;
    QMap<QString,CanDataValue> dataMap= canReader.getValues(data);
    qDebug()<<"dataMap"<<dataMap.count();

    DeviceStatusInfo deviceIndo;
    deviceIndo.dateTime=TimeFormatTrans::convertToLongDateTime(data.dateTime);
    deviceIndo.deviceStatus.Status=dataMap["WorkStatus"].value.toUInt();
    deviceIndo.deviceStatus.deviceAddress=data.dataid & 0xFF;

    memcpy( &deviceIndo.deviceStatus.faultInfo,data.data+1,7);

    //判断状态是否变化
    if(devices[data.dataid]->refreshStat(deviceIndo))
    {
        int statId= saveStat(deviceIndo);
        if(statId>0)
            saveDeviceStatSignals(statId,dataMap);
    }
}

//保存到数据库
int DeviceManager::saveStat(const DeviceStatusInfo &deviceIndo)
{
    return   dbOperator->insertDeviceStatusInfo(deviceIndo);
}

void DeviceManager::saveDeviceStatSignals(int statID,const QMap<QString,CanDataValue> &dataMap)
{
    for(auto it=dataMap.begin();it!=dataMap.end();++it)
    {

        DeviceErrorInfo errorInfo;
        errorInfo.statId=statID;
        errorInfo.errorInfo=QString("%1: %2 %3").arg(it.key()).arg(it.value().value.toInt()).arg(it.value().valueDescription);
        dbOperator->insertDeviceErrorInfo(errorInfo);

        qDebug()<<it.key()<<it.value().value.toInt()<<it.value().valueDescription;
    }
}

//获取设备列表
void DeviceManager::getDeviceLists()
{
    for(const CanDataFormat &device:canReader.getCanDataList())
    {
        DeviceStat *deviceStat=new DeviceStat(device.id,this);
        connect(deviceStat,&DeviceStat::sig_WorkTimeRecord,this,&DeviceManager::recordWorkTime);
        connect(deviceStat,&DeviceStat::sig_StatChanged,this,&DeviceManager::StatWorkChange);
        devices[device.id]=deviceStat;
    }
    qDebug()<<"deviceList"<<devices.count();
}

void DeviceManager::recordWorkTime(int deviceID)
{
    dbOperator->updateDeviceWorkTimeAdd1Minute(deviceID);
}

void DeviceManager::StatWorkChange(DeviceStatusInfo deviceInfo)
{
    saveStat(deviceInfo);
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
                DeviceTotalWorkTime workTimeInfo;
                workTimeInfo.deviceId=device.deviceAddress;
                workTimeInfo.totalWorkTime=0;
                // 插入设备工作时间到数据库
                if (!dbOperator->insertDeviceTotalWorkTime(workTimeInfo)) {
                    qDebug() << "Failed to insert device worktime:" << device.deviceName;
                } else {
                    qDebug() << "Inserted device worktime:" << device.deviceName << "with address" << addressStr;
                }
            }
        }
    }
    file.close();
    return true;
}

//获取当前故障设备信息
QByteArray DeviceManager::getErrorDeviceStat(int &deviceCount)
{
    QByteArray array;
    deviceCount = 0;
    for(auto it = devices.begin(); it != devices.end(); ++it)
    {
        if(it.value() != nullptr) // 检查指针合法性
        {
            DeviceStatus deviceStatus = it.value()->workStatus().deviceStatus;
            if(deviceStatus.Status!=0x0F)
            {
                array.append(QByteArray(reinterpret_cast<const char*>(&deviceStatus), sizeof(DeviceStatus)));
                deviceCount++;
            }
        }
    }
    return array;
}

//根据设备地址获取设备信息
QByteArray DeviceManager::getDeviceStat(int deviceAddress, int &deviceCount)
{
    QByteArray array;
    deviceCount = 0;

    for(auto it = devices.begin(); it != devices.end(); ++it)
    {
        if(it.value() != nullptr) // 检查指针合法性
        {
            if(deviceAddress == 0 || (it.key() & 0xFF) == deviceAddress) // 优先级修正
            {
                DeviceStatus deviceStatus = it.value()->workStatus().deviceStatus;
                array.append(QByteArray(reinterpret_cast<const char*>(&deviceStatus), sizeof(DeviceStatus)));
                deviceCount++;
                if(deviceAddress != 0) // 如果找到指定设备，直接返回
                {
                    break;
                }
            }
        }
    }
    return array;
}

//根据设备地址获取设备故障信息
QList<DeviceStatusInfo> DeviceManager::getHistoryDeviceStat(int deviceAddress,TimeCondition *timeConditionPtr)
{
    QList<DeviceStatusInfo> statInfoList;
    QList<DeviceStatusInfo> statInfoListTemp= dbOperator->getDeviceStatusInfos(timeConditionPtr);
    for(const DeviceStatusInfo &statInfo:statInfoListTemp)
    {
        if(deviceAddress==0 || deviceAddress==statInfo.deviceStatus.deviceAddress)
        {
            statInfoList.append(statInfo);
        }

    }
    return statInfoList;
}


//根据设备地址获取设备累计工作时间
QByteArray DeviceManager::getDeviceTotalWorktime(int deviceAddress, int &deviceCount)
{
    QByteArray array;
    deviceCount = 0;

    for(auto it = devices.begin(); it != devices.end(); ++it)
    {
        if(it.value() != nullptr) // 检查指针合法性
        {
            if(deviceAddress == 0 || (it.key() & 0xFF) == deviceAddress) // 优先级修正
            {
                DeviceTotalWorkTime deviceWorkTime = it.value()->deviceWorkTime();
                array.append(QByteArray(reinterpret_cast<const char*>(&deviceWorkTime), sizeof(DeviceTotalWorkTime)));
                deviceCount++;
                if(deviceAddress != 0) // 如果找到指定设备，直接返回
                {
                    break;
                }
            }
        }
    }
    return array;
}
>>>>>>> a47cf4023014f66bb831bba7d44fd896e866a872
