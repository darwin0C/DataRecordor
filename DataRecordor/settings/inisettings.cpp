#include "inisettings.h"
#include "data.h"

#include <QCoreApplication>

iniSettings::iniSettings(QObject *parent) : QObject(parent)
{
    tarIdentType=0;
    filePath = QCoreApplication::applicationDirPath()+"/data/setting.ini";
    dataInitial();
}

void iniSettings::dataInitial()
{
    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);
    if(file.exists())
    {

        //加载网络设置
        loadNetSettings(mysetting);
        //加载语言设置
        loadLanguage(mysetting);

        loadRadarDataFlag(mysetting);
    }
    else
    {
        //加载单位设置
        mysetting->beginGroup("CIRCLE");
        mysetting->setValue("Circle","6000");

        mysetting->endGroup();
        //加载语言设置
        mysetting->beginGroup("LANGUAGE");
        uint lan=mysetting->value("Language","1").toInt();
        mysetting->endGroup();
    }
    delete mysetting;
    mysetting=NULL;
}
///加载单位设置
void iniSettings::loadUnitSet(QSettings *mysetting)
{
    mysetting->beginGroup("CIRCLE");
    if(mysetting->contains("North"))
    {
        northData=mysetting->value("North","2000").toInt();
    }
    else
    {
        mysetting->setValue("North","2000");
        northData=mysetting->value("North","2000").toInt();
    }
    mysetting->endGroup();
}
///加载扫描速度
void iniSettings::loadScanSpeed(QSettings *mysetting)
{
    //加载扫描速度
    mysetting->beginGroup("SCAN_SPEED");
    if(mysetting->contains("scanSpeed"))
    {
        scanSpeed=mysetting->value("scanSpeed","10").toInt();
    }
    else
    {
        mysetting->setValue("scanSpeed","10");
        scanSpeed=mysetting->value("scanSpeed","10").toInt();
    }
    //scanMaxSpeed
    if(mysetting->contains("scanMaxSpeed"))
    {
        scanMaxSpeed=mysetting->value("scanMaxSpeed","150").toInt();
    }
    else
    {
        mysetting->setValue("scanMaxSpeed","150");
        scanMaxSpeed=150;
    }
    mysetting->endGroup();
}
///最大工作时间设置
void iniSettings::loadDevTotalWorktime(QSettings *mysetting)
{
    //加载最大连续测距时间
    mysetting->beginGroup("MAX_TIME_LIMIT");
    if(mysetting->contains("LasarRangeTime"))
    {
        LasarRangeTime=mysetting->value("LasarRangeTime","180").toInt();
    }
    else
    {
        mysetting->setValue("LasarRangeTime","180");
        LasarRangeTime=180;
    }
    //设备累计工作时间
    if(mysetting->contains("DeviceTotalWorkTime"))
    {
        DeviceTotalWorkTime=mysetting->value("DeviceTotalWorkTime","0").toInt();
    }
    else
    {
        mysetting->setValue("DeviceTotalWorkTime","0");
        DeviceTotalWorkTime=0;
    }
    mysetting->endGroup();
}
///加载拼接速度
void iniSettings::loadImgJointSpeed(QSettings *mysetting)
{
    mysetting->beginGroup("IMG_JOINT_SPEED");
    if(mysetting->contains("minImgJointSpeed"))
    {
        minImgJointSpeed=mysetting->value("minImgJointSpeed","40").toInt();
    }
    else
    {
        mysetting->setValue("minImgJointSpeed","40");
        minImgJointSpeed=40;
    }
    //
    if(mysetting->contains("maxImgJointSpeed"))
    {
        maxImgJointSpeed=mysetting->value("maxImgJointSpeed","150").toInt();
    }
    else
    {
        mysetting->setValue("maxImgJointSpeed","150");
        maxImgJointSpeed=150;
    }
    mysetting->endGroup();
}
//const QString PASSWORD="12345";
///加载密码设置
void iniSettings::loadPassWord(QSettings *mysetting)
{


    mysetting->beginGroup("PASSWORD");
    if(mysetting->contains("passWord"))
        passWord=mysetting->value("passWord","12345").toString();
    else
    {
        mysetting->setValue("passWord","12345");
        passWord="12345";
    }
    mysetting->endGroup();
}

///加载语言设置
void iniSettings::loadLanguage(QSettings *mysetting)
{

    uint lan=0;
    mysetting->beginGroup("LANGUAGE");
    if(mysetting->contains("Language"))
        lan=mysetting->value("Language","1").toInt();
    else
    {
        mysetting->setValue("Language","1");
        lan=1;
    }
    mysetting->endGroup();
}
///加载目标识别类型设置
void iniSettings::loadTarIndentType(QSettings *mysetting)
{

    mysetting->beginGroup("TARIDENTTYPE");
    if(mysetting->contains("tarIdenType"))
        tarIdentType=mysetting->value("tarIdenType","1").toInt();
    else
    {
        mysetting->setValue("tarIdenType","1");
        tarIdentType=1;
    }
    mysetting->endGroup();
}
//加载姿态源设置
void iniSettings::loadPosSource(QSettings *mysetting)
{
    mysetting->beginGroup("POSSOURCE");
    if(mysetting->contains("PosSource"))
        posSource=mysetting->value("PosSource","1").toInt();
    else
    {
        mysetting->setValue("PosSource","1");
        posSource=1;
    }
    mysetting->endGroup();
}
///加载伺服姿态源设置
void iniSettings::loadServPosSource(QSettings *mysetting)
{

    mysetting->beginGroup("SERVANGLE");
    if(mysetting->contains("SevAngleSource"))
        sevAngleSource=mysetting->value("SevAngleSource","1").toInt();
    else
    {
        mysetting->setValue("SevAngleSource","1");
        sevAngleSource=1;
    }
    mysetting->endGroup();
}
//加载坐标显示方式设置
void iniSettings::loadPosShowType(QSettings *mysetting)
{

    mysetting->beginGroup("POSSHOWTYPE");
    if(mysetting->contains("posShowType"))
        posShowType=mysetting->value("posShowType","1").toInt();
    else
    {
        mysetting->setValue("posShowType","1");
        posShowType=1;
    }
    mysetting->endGroup();
}
///加载网络设置
void iniSettings::loadNetSettings(QSettings *mysetting)
{
    mysetting->beginGroup("NETSETTING");

    //selfNetPort
    if(mysetting->contains("selfNetPort"))
        selfNetPort=mysetting->value("selfNetPort","65110").toInt();
    else
    {
        mysetting->setValue("selfNetPort","65110");
        selfNetPort=65110;
    }
    //DeviceIP
    if(mysetting->contains("DeviceIP"))
        DeviceIP=mysetting->value("DeviceIP","192.168.9.207").toString();
    else
    {
        mysetting->setValue("DeviceIP","192.168.9.207");
        DeviceIP="192.168.9.207";
    }
    //DevicePort
    if(mysetting->contains("DevicePort"))
        DevicePort=mysetting->value("DevicePort","65111").toInt();
    else
    {
        mysetting->setValue("DevicePort","65111");
        DevicePort=65111;
    }
    mysetting->endGroup();
}


void iniSettings::loadRadarDataFlag(QSettings *mysetting)
{
    mysetting->beginGroup("RadarDataSet");
    if(mysetting->contains("radarDataFlag"))
        radarDataFlag=mysetting->value("radarDataFlag","0").toInt();
    else
    {
        mysetting->setValue("radarDataFlag","0");
        radarDataFlag=0;
    }
    mysetting->endGroup();
}
int iniSettings::getRadarDataFlagLimit()
{
    return radarDataFlag;
}

//获取指挥网络参数
void iniSettings::getCommandNet(QString &ip,int & port)
{
    ip=commandIP;
    port=commandPort;
}
//获取驱鸟网络参数
void iniSettings::getExpelBirdNet(QString &ip,int & port)
{
    ip=expelBirdIP;
    port=expelBirdPort;
}
//获取驱鸟网络参数
void iniSettings::getExpelBirdTcpNet(QString &ip,int & port)
{
    ip=expelBirdIP;
    port=expelBirdTcpPort;
}

//获取反无网络参数
void iniSettings::getAntiUAVNet(QString &ip,int & port)
{
    ip=antiUAVIP;
    port=antiUAVPort;
}
//获取运维网络参数
void iniSettings::getOandMNet(QString &ip,int & port)
{
    ip=OandM_IP;
    port=OandM_Port;
}
//获取自身网络参数
void iniSettings::getSelfNet(int &port)
{
    port=selfNetPort;
}
//获取设备网络参数
void iniSettings::getDeviceNetData(QString &ip,int & port)
{
    ip=DeviceIP;
    port=DevicePort;
}
//获取扫描速度数据
int iniSettings::getSCANSpeed()
{
    return scanSpeed;
}
int iniSettings::getSCANMaxSpeed()
{
    return scanMaxSpeed;
}
//获取图像拼接速度
int iniSettings::getMinImgJointSpeed()
{
    return minImgJointSpeed;
}
int iniSettings::getMaxImgJointSpeed()
{
    return maxImgJointSpeed;
}

void iniSettings::setCircleNum(int circle)
{

    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);
    //加载单位设置
    mysetting->beginGroup("CIRCLE");
    mysetting->setValue("Circle",circle);
    mysetting->endGroup();
    delete mysetting;
    mysetting=NULL;
}
void iniSettings::SaveMannualPos(double lan,double lon,double h,int coorType,int nsFlag)
{

    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);

    mysetting->beginGroup("GPS");
    mysetting->setValue("gps_lan",lan);
    mysetting->setValue("gps_lon",lon);
    mysetting->setValue("gps_height",h);
    mysetting->setValue("coorType",coorType);
    mysetting->setValue("n_sFlag",nsFlag);
    mysetting->endGroup();

    delete mysetting;
    mysetting=NULL;
}

void iniSettings::saveServAngleSource(int source)
{

    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);
    //加载单位设置
    mysetting->beginGroup("SERVANGLE");
    mysetting->setValue("SevAngleSource",source);
    mysetting->endGroup();
    delete mysetting;
    mysetting=NULL;
}

void iniSettings::savePosSource(int source)
{

    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);
    //加载单位设置
    mysetting->beginGroup("POSSOURCE");
    mysetting->setValue("PosSource",source);
    mysetting->endGroup();
    delete mysetting;
    mysetting=NULL;
}

void iniSettings::saveNorthData(int north)
{

    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);
    //加载单位设置
    mysetting->beginGroup("CIRCLE");
    mysetting->setValue("North",north);
    mysetting->endGroup();
    delete mysetting;
    mysetting=NULL;
}


int iniSettings::curServAngleSource()
{
    return sevAngleSource;
}
int iniSettings::curNorth()
{
    return northData;
}
int iniSettings::curPosSource()
{
    return posSource;
}
int iniSettings::getIdentType()
{
    return tarIdentType;
}
//坐标显示方式 0：XYH 1：经纬度
int iniSettings::posType()
{
    return posShowType;
}
void iniSettings::setIdentType(int type)
{
    tarIdentType=type;
    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);
    mysetting->beginGroup("TARIDENTTYPE");
    mysetting->setValue("tarIdenType",tarIdentType);
    mysetting->endGroup();
    delete mysetting;
    mysetting=NULL;
}
//最大连续测距时间
int iniSettings::getMaxLasarRangeTime()
{
    return LasarRangeTime;
}
//获取设备最大累计工作时间
int iniSettings::getDeviceTolWorkTime()
{
    return DeviceTotalWorkTime;
}
//获取密码
QString iniSettings::getSysPassWord()
{
    return passWord;
}

void iniSettings::setDeviceTolWorkTime(int workTime)
{
    DeviceTotalWorkTime=workTime;
    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);
    //加载最大连续测距时间
    mysetting->beginGroup("MAX_TIME_LIMIT");
    mysetting->setValue("DeviceTotalWorkTime",workTime);
    mysetting->endGroup();
    delete mysetting;
    mysetting=NULL;
}
