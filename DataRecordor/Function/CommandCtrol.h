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
    QString selfUniqueID;
    quint16 commandCode=0xE0;

    void sysTimeSetHandle(const TimeSetCMD &commandData);
    void dataRequreHandle(const CommandDataRequre &commandData);
    QByteArray requreCurrentData(int dataFlag,int deviceAddress);

    void SendRTDataToCommand(int dataFlag,int deviceAddress);

    void sendHistoryDataToCommand(int dataFlag, int deviceAddress, TimeCondition *timeConditionPtr);
    void sendHisData(Send2CommandData sendData, int dataFlag, int deviceAddress, TimeCondition *timeConditionPtr);
    void initData();
    void setAttributeHandle(const SelfAttributeData &commandData);
    bool stringToCharArray(const QString &source, char *dest, int maxSize);
signals:

private slots:
    void dataHandle(quint16 sendCode,quint16 cmdCode,QByteArray array);
    void timeStatHandle();
    void sendCommandData(Send2CommandData sendData, QByteArray dataArray);
    void autoSendCommandDataHandle(int dataFlag, QByteArray dataArray);
    void setAutoReport(bool enable);
};

#endif // COMMANDCTROL_H
