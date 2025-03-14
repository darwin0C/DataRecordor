#ifndef EVENTINFO_H
#define EVENTINFO_H

#include <QObject>
#include "DeviceData.h"
#include "data.h"
#include "canmsgreader.h"
#include "dboperator.h"
#include "CommandData.h"
#include <QTimer>
class EventInfo : public QObject
{
    Q_OBJECT
public:
    explicit EventInfo(QObject *parent = nullptr);

    GunMoveData getGunMoveData();
    GunFiringData getGunFiringData();
    QList<GunMoveData> getHistoryGunMoveData(TimeCondition *timeConditionPtr);
    QList<GunFiringData> getHistoryGunFiringData(TimeCondition *timeConditionPtr);
    void setAutoReport(bool enable);
    QByteArray getCurrentAlarmData();
private:
    CanMsgReader canReader;
    QMap<int,QString> eventList;

    GunMoveData gunMoveData;

    qint16 propellantTemperature=0;
    GunAttitudeData gunAttitude;
    GunFiringData gunFiringData;

    AlarmInfo nuclearBioAlarmInfo;
    AlarmInfo fireSuppressBioAlarmInfo;

    QTimer alarmTimer;

    bool isInitSpeedreceived=false;
    bool isAutoSendEnabled=true;

    quint64 nuclearBioAlarmCount=5;
    quint64 fireSuppressAlarmCount=5;

    bool isRevShootSignal=false;

    void getEventList();
    void refrushStat(int dataId, const QMap<QString, CanDataValue> &dataMap,LocalDateTime dateTime);
    void test();
    void saveGunMovedata();
    void saveGunFiringData();
    void saveAlarmInfo(const AlarmInfo &alarmInfo);

    int SendGunFiringData(GunFiringData gunfirngData);
    void deviceDataHandle(const CanData &data);
    void GunnerBroadcastRTSDataHandle(unsigned int canID, uchar *buff);
    void GunnerBroadcastDTDataHandle(uchar *buff);
    void GunnerP2PLongDataCTSData(uint canId, uchar *buff);

    void resbanceToGunner(uchar *buff,int len);
private slots:
    void onTimeout();
    void processCanData(const CanData &data);
    void alarmOntimeHandle();
    void canLongDataHandle(unsigned char *png, unsigned char *buff, unsigned short len, bool bbroadcast);
signals:
    void sendCommandDataSig(int, QByteArray);
};

#endif // EVENTINFO_H
