#include "inisettings.h"
#include "data.h"

#include <QCoreApplication>

iniSettings::iniSettings(QObject *parent) : QObject(parent)
{

    filePath = QCoreApplication::applicationDirPath()+GlobSettingFile;
    dataInitial();
}

void iniSettings::dataInitial()
{
    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);

    //加载网络设置
    loadNetSettings(mysetting);
    //加载属性设置
    loadAttribute(mysetting);

    delete mysetting;
    mysetting=NULL;
}

///****************************************///
///加载配置
///****************************************///
//加载密码设置
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
void iniSettings::loadAttribute(QSettings *mysetting)
{
    mysetting->beginGroup("ATTRIBUTE");
    if(mysetting->contains("Self_Attribute"))
        selfAttribute=mysetting->value("Self_Attribute","123").toUInt();
    else
    {
        mysetting->setValue("Self_Attribute","123");
        selfAttribute=123;
    }

    if(mysetting->contains("Self_UniqueID"))
        selfUniqueID=mysetting->value("Self_UniqueID","123456").toUInt();
    else
    {
        mysetting->setValue("Self_UniqueID","123456");
        selfUniqueID="123456";
    }

    mysetting->endGroup();
}

///加载网络设置
void iniSettings::loadNetSettings(QSettings *mysetting)
{
    mysetting->beginGroup("NET_SETTING");

    //selfNetPort
    if(mysetting->contains("Self_Net_Port"))
        selfNetPort=mysetting->value("Self_Net_Port","65110").toInt();
    else
    {
        mysetting->setValue("Self_Net_Port","65110");
        selfNetPort=65110;
    }
    //Command_IP
    if(mysetting->contains("Command_IP"))
        commandIP=mysetting->value("Command_IP","192.168.16.116").toString();
    else
    {
        mysetting->setValue("Command_IP","192.168.16.116");
        commandIP="192.168.16.116";
    }
    //commandPort
    if(mysetting->contains("Command_Port"))
        commandPort=mysetting->value("Command_Port","6800").toInt();
    else
    {
        mysetting->setValue("Command_Port","6800");
        commandPort=65111;
    }
    mysetting->endGroup();
}
///****************************************///
///获取配置参数
///****************************************///

//获取指挥网络参数
void iniSettings::getCommandNet(QString &ip,int & port)
{
    ip=commandIP;
    port=commandPort;
}

//获取自身网络参数
void iniSettings::getSelfNet(int &port)
{
    port=selfNetPort;
}
//获取设备网络参数
void iniSettings::getDeviceNetData(QString &ip,int &port)
{
    ip=DeviceIP;
    port=DevicePort;
}

//获取密码
QString iniSettings::getSysPassWord()
{
    return passWord;
}

 quint16 iniSettings::getSelfAttribute()
 {
     return selfAttribute;
 }

 QString iniSettings::getSelfUniqueID()
 {
     return selfUniqueID;
 }

///****************************************///
///保存配置
///****************************************///
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


void iniSettings::setAttribute(quint16 attribute,QString uniqueId)
{
    QFile file(filePath);
    QSettings *mysetting=new QSettings(filePath,QSettings::IniFormat);

    mysetting->beginGroup("ATTRIBUTE");
    mysetting->setValue("Self_Attribute",attribute);
    selfAttribute=attribute;

    mysetting->setValue("Self_UniqueID",uniqueId);
    selfUniqueID=uniqueId;

    mysetting->endGroup();
    delete mysetting;
    mysetting=NULL;
}

