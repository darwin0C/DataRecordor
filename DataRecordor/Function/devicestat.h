#ifndef DEVICESTAT_H
#define DEVICESTAT_H

#include <QObject>
#include "DeviceData.h"
#include <QTimer>

class DeviceStat : public QObject
{
    Q_OBJECT
public:
    explicit DeviceStat(QObject *parent = nullptr);

    void deviceDataHandle(uint id,unsigned char *buff);
    int LinkStat();

private slots:
    void timerStatHandle();
private:
    void StatDataHandle(unsigned char *buff);
    void refreshStat(const DeviceStatusInfo &Stat);
    QTimer *mytimer=nullptr;
    int LinkCount;
    Device_Stat DeviceLinkStat;
    DeviceStatusInfo lastDeviceStat;

signals:
    void sig_StatChanged(DeviceStatusInfo);
};

#endif // DEVICESTAT_H
