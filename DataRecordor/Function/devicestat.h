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
    bool refreshStat(const DeviceStatusInfo &Stat);
private slots:
    void timerStatHandle();
private:
    void StatDataHandle(unsigned char *buff);

    QTimer *mytimer=nullptr;
    int LinkCount;
    Device_Stat DeviceLinkStat;
    DeviceStatusInfo lastDeviceStat;
    // 获取当前时间作为起始时间点
    QDateTime startTime;

    int workTime=0;
    bool isWorking=false;
    void recoardWorkTime(QDateTime endTime);
signals:
    void sig_StatChanged(DeviceStatusInfo);
    void sig_WorkTimeRecord(int);
};

#endif // DEVICESTAT_H
