#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include "canmsgreader.h"
#include "data.h"
#include "dboperator.h"
#include "devicestat.h"

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    void processCanData(const CanData &data);
private:
    CanMsgReader canReader;
    QList<CanDataFormat> canDataList;
    //QList<int> deviceList;
    QMap<int,DeviceStat*> devices;
    void getDeviceLists();
    DbOperator *dbOperator;
    bool loadAndInsertDevicesFromXml(const QString &xmlFilePath);
    void saveDeviceStatSignals(int statID,const QMap<QString, CanDataValue> &dataMap);
    void test();
    int saveStat(const DeviceStatusInfo &deviceIndo);

signals:


private slots:
    void recordWorkTime(int deviceID);
    void StatWorkChange(DeviceStatusInfo deviceInfo);
};

#endif // DEVICEMANAGER_H
