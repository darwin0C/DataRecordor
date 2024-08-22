#ifndef COMMANDCTROL_H
#define COMMANDCTROL_H

#include <QObject>
#include <QTimer>
#include "DeviceManager.h"
#include "EventInfo.h"
#include "CommandData.h"

class CommandCtrol : public QObject
{
    Q_OBJECT
public:
    explicit CommandCtrol(QObject *parent = nullptr);

private:
    QTimer statTimer;

    DeviceManager deviceManager;
    EventInfo eventInfo;

    quint16 selfAttribute;
    quint32 selfUniqueID;

    void setAutoReport(bool enable);
    void sysTimeSetHandle(const TimeSetCMD &commandData);
    void dataRequreHandle(const CommandDataRequre &commandData);
    QByteArray requreCurrentData(int dataFlag,int deviceAddress);

    void SendRTDataToCommand(int dataFlag,int deviceAddress);

    void sendHistoryDataToCommand(int dataFlag, int deviceAddress, TimeCondition *timeConditionPtr);
    void sendHisData(Send2CommandData sendData, int dataFlag, int deviceAddress, TimeCondition *timeConditionPtr);
    void initData();
signals:

private slots:
    void dataHandle(QByteArray array);
    void timeStatHandle();
    void sendCommandData(Send2CommandData sendData, QByteArray dataArray);
    void autoSendCommandDataHandle(int dataFlag, QByteArray dataArray);
};

#endif // COMMANDCTROL_H
