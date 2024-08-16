#ifndef DEVICESTAT_H
#define DEVICESTAT_H

#include <QObject>
#include "canmsgreader.h"
#include "data.h"

class DeviceStat : public QObject
{
    Q_OBJECT
public:
    explicit DeviceStat(QObject *parent = nullptr);
    void processCanData(const CanData &data);
private:
    CanMsgReader canReader;
    QList<CanDataFormat> canDataList;
    QList<int> deviceList;
    void getDeviceLists();
signals:

};

#endif // DEVICESTAT_H
