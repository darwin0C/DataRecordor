#ifndef INISETTINGS_H
#define INISETTINGS_H

#include <QObject>
#include <QSettings>
#include <QFile>
#include <QDebug>


class iniSettings : public QObject
{
    Q_OBJECT
public:
    explicit iniSettings(QObject *parent = nullptr);
    static iniSettings *Get()
    {
        static iniSettings vt;
        return &vt;
    }
    void dataInitial();
    void setCircleNum(int circle);

    void getCommandNet(QString &ip, int &port);
    void getSelfNet(int &port);
    void getDeviceNetData(QString &ip, int &port);

    QString getSysPassWord();

    void setAttribute(quint16 selfAttribute);
    void setUniqueID(quint32 uniqueId);
    quint16 getSelfAttribute();
    quint32 getSelfUniqueID();
private:

    QString passWord;

    QString commandIP;
    int commandPort;

    int selfNetPort;

    QString DeviceIP;
    int DevicePort;

    QString filePath;

    quint16 selfAttribute;//本车属性
    quint32 selfUniqueID;//本车唯一ID


    void loadRadarDataFlag(QSettings *mysetting);
    void loadNetSettings(QSettings *mysetting);
    void loadPassWord(QSettings *mysetting);
    void loadAttribute(QSettings *mysetting);
signals:

};

#endif // INISETTINGS_H
