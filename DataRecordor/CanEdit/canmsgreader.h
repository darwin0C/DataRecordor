#ifndef CANMSGREADER_H
#define CANMSGREADER_H

#include <QObject>
#include "DeviceData.h"

class CanMsgReader : public QObject
{
    Q_OBJECT
    void parseCanData(const QByteArray &canFrame, const QList<CanDataFormat> &canDataList, int canId);
    quint64 extractBits(const QByteArray &data, int startBit, int length);

public:
    explicit CanMsgReader(QObject *parent = nullptr);
    bool readCanDataFromXml(QList<CanDataFormat> &canDataList, const QString &fileName);
signals:

};

#endif // CANMSGREADER_H
