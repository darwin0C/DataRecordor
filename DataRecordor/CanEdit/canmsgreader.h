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
    QList<CanDataFormat> canDataList;
public:
    explicit CanMsgReader(QObject *parent = nullptr);
    bool readCanDataFromXml(const QString &fileName);
    QMap<QString,CanDataValue> getValues(const CanData &data);
    QList<CanDataFormat> getCanDataList();
signals:

};

#endif // CANMSGREADER_H
