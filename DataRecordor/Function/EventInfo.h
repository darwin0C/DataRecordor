#ifndef EVENTINFO_H
#define EVENTINFO_H

#include <QObject>
#include "DeviceData.h"
#include "data.h"
#include "canmsgreader.h"
#include "dboperator.h"

class EventInfo : public QObject
{
    Q_OBJECT
public:
    explicit EventInfo(QObject *parent = nullptr);

private:
    CanMsgReader canReader;
    QList<CanDataFormat> canDataList;

    QMap<int,QString> eventList;

    GunMoveData gunMoveData;
    GunFiringData gunFiringData;

    bool isInitSpeedreceived=false;

    void getEventList();
    void refrushStat(int dataId, const QMap<QString, CanDataValue> &dataMap,LocalDateTime dateTime);
    void test();
    void saveGunMovedata();
    void saveGunFiringData();
public slots:
    void processCanData(const CanData &data);
private slots:
    void onTimeout();
signals:
};

#endif // EVENTINFO_H
