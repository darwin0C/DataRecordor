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
    void requreCurrentData(int dataType);
    void requreHistoryData(int dataType, TimeCondition *timeConditionPtr);
signals:

private slots:
    void dataHandle(QByteArray array);
    void timeStatHandle();
};

#endif // COMMANDCTROL_H
