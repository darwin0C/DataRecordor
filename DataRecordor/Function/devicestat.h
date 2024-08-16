#ifndef DEVICESTAT_H
#define DEVICESTAT_H

#include <QObject>

class DeviceStat : public QObject
{
    Q_OBJECT
public:
    explicit DeviceStat(QObject *parent = nullptr);

signals:

};

#endif // DEVICESTAT_H
