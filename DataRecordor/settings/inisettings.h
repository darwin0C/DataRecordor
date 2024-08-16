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
    void SaveMannualPos(double lan, double lon, double h, int coorType,int nsFlag);
    int curServAngleSource();
    int curPosSource();
    void saveServAngleSource(int source);
    void savePosSource(int source);
    int getIdentType();
    void setIdentType(int type);
    int curNorth();
    void saveNorthData(int north);

    void getCommandNet(QString &ip, int &port);
    void getSelfNet(int &port);
    void getDeviceNetData(QString &ip, int &port);
    void getExpelBirdNet(QString &ip, int &port);
    void getAntiUAVNet(QString &ip, int &port);
    int getSCANSpeed();
    int getSCANMaxSpeed();
    void getOandMNet(QString &ip, int &port);
    int posType();
    int getMaxLasarRangeTime();
    int getDeviceTolWorkTime();
    void setDeviceTolWorkTime(int workTime);

    int getMinImgJointSpeed();
    int getMaxImgJointSpeed();
    int getRadarDataFlagLimit();
    void getExpelBirdTcpNet(QString &ip, int &port);
    QString getSysPassWord();
private:
    int posSource;
    int posShowType=0;
    int sevAngleSource;
    int tarIdentType;
    QString passWord;
    int northData;
    QString commandIP;
    int commandPort;
    QString antiUAVIP;
    int antiUAVPort;
    QString OandM_IP;
    int OandM_Port;
    QString expelBirdIP;
    int expelBirdPort;
    int expelBirdTcpPort=8080;
    int selfNetPort;
    QString DeviceIP;
    int DevicePort;
    int radarDataFlag=0;

    int scanSpeed=5;
    int scanMaxSpeed=120;

    int minImgJointSpeed=40;
    int maxImgJointSpeed=150;

    QString filePath;
    int LasarRangeTime;
    int DeviceTotalWorkTime;
    void loadRadarDataFlag(QSettings *mysetting);
    void loadNetSettings(QSettings *mysetting);


    void loadLanguage(QSettings *mysetting);
    void loadPosSource(QSettings *mysetting);
    void loadPosShowType(QSettings *mysetting);
    void loadServPosSource(QSettings *mysetting);
    void loadTarIndentType(QSettings *mysetting);
    void loadImgJointSpeed(QSettings *mysetting);
    void loadDevTotalWorktime(QSettings *mysetting);
    void loadScanSpeed(QSettings *mysetting);
    void loadUnitSet(QSettings *mysetting);
    void loadPassWord(QSettings *mysetting);
signals:

};

#endif // INISETTINGS_H
