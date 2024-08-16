#ifndef CANMSGREADER_H
#define CANMSGREADER_H

#include <QObject>
#include "DeviceData.h"
#include "data.h"

class CanMsgReader : public QObject
{
    Q_OBJECT
    QMap<QString,CanDataValue> parseCanData(const QByteArray &canFrame, const QList<CanDataFormat> &canDataList, int canId);
    quint64 extractBits(const QByteArray &data, int startBit, int length);

public:
    explicit CanMsgReader(QObject *parent = nullptr);
    bool readCanDataFromXml(QList<CanDataFormat> &canDataList, const QString &fileName);
    QMap<QString,CanDataValue> getValues(const CanData &data, const QList<CanDataFormat> &canDataList);
signals:

};

#endif // CANMSGREADER_H
