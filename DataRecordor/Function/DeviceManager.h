#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include "canmsgreader.h"
#include "data.h"
#include "dboperator.h"
#include "devicestat.h"
#include "CommandData.h"

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);

    QByteArray getDeviceStat(int deviceAddress,int &deviceCount);
    QByteArray getErrorDeviceStat(int &deviceCount);
    QByteArray getDeviceTotalWorktime(int deviceAddress, int &deviceCount);
    QList<DeviceStatusInfo> getHistoryDeviceStat(int deviceAddress, TimeCondition *timeConditionPtr);
private:
    CanMsgReader canReader;
    //QList<CanDataFormat> canDataList;
    //QList<int> deviceList;
    QMap<int,DeviceStat*> devices;
    void getDeviceLists();
    DbOperator *dbOperator;
    bool loadAndInsertDevicesFromXml(const QString &xmlFilePath);
    void saveDeviceStatSignals(int statID,const QMap<QString, CanDataValue> &dataMap);
    void test();
    int saveStat(const DeviceStatusInfo &deviceIndo);

signals:
    void sendCommandDataSig(Send2CommandData, QByteArray);

private slots:
    void recordWorkTime(int deviceID);
    void StatWorkChange(DeviceStatusInfo deviceInfo);
    void processCanData(const CanData &data);
};

#endif // DEVICEMANAGER_H
