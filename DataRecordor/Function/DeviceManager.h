#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include "canmsgreader.h"
#include "data.h"
#include "dboperator.h"

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    explicit DeviceManager(QObject *parent = nullptr);
    void processCanData(const CanData &data);
private:
    CanMsgReader canReader;
    QList<CanDataFormat> canDataList;
    QList<int> deviceList;
    void getDeviceLists();
    DbOperator *dbOperator;
    bool loadAndInsertDevicesFromXml(const QString &xmlFilePath);
    void saveDeviceStat(const QMap<QString, CanDataValue> &dataMap);
    void test();
signals:


};

#endif // DEVICEMANAGER_H
