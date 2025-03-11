#include "commanager.h"

#include <QFile>
#include <QDir>
#include <QSettings>
#include <QSerialPortInfo>
#include "inisettings.h"
#include "MsgSignals.h"


ComManager::ComManager(QObject *parent) : QObject(parent)
{
    QStringList Seriallist;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        qDebug()<<"info.portName===="<<info.portName();
        Seriallist<<info.portName();
    }
#ifdef SERIALCOM
    if(Seriallist.count()>3)
    {
        try
        {
            serialCom1= startSerial(Seriallist[1]);
            serialCom2= startSerial(Seriallist[2]);
            serialCom3= startSerial(Seriallist[3]);
        }
        catch (std::exception &e)
        {

        }
    }

#endif
#ifdef NET_COM
    myComBdCastSocketPort=6800;  //广播端口号

    //mySelfSocketPort=65110;  //自己的端口号
    mySelfSocketIP="192.168.9.33";    //自己的IP
    iniSettings::Instance()->getSelfNet(mySelfSocketPort);

    iniSettings::Instance()->getCommandNet(netSocketIP,netSocketPort);

    myNetComInterface=new QMyNetCom();
    myNetComInterface->initSocket(mySelfSocketPort);

    if(myNetComInterface->bcommandudpopen)
        myNetComInterface->start();

    myNetBdCastInterface=new QMyNetCom();
    myNetBdCastInterface->initSocket(myComBdCastSocketPort);
    if(myNetBdCastInterface->bcommandudpopen)
        myNetBdCastInterface->start();

    int groupPort=6012;
    myGroupCastInterface=new QMyNetCom();
    myGroupCastInterface->initSocket(groupPort);
    myGroupCastInterface->joinGroup("225.0.0.12");
    //connect(myGroupCastInterface,SIGNAL(sendQByteArraySig(QByteArray)),this,SLOT(netDataHandle(QByteArray)));
    if(myGroupCastInterface->bcommandudpopen)
        myGroupCastInterface->start();

#endif

}

QMyCom *ComManager::startSerial(QString portNum)
{
    QMyCom *serialCom=new QMyCom();
    connect(this,&ComManager::sendCanMegSig,serialCom,&QMyCom::sendCanMegSigHandle);
    if(serialCom->initComInterface(portNum,921600))
    {
        serialCom->start();
    }
    return serialCom;
}

ComManager::~ComManager()
{

}

//int ComManager::sendRecordCanData(uint canID,uchar *buff,unsigned char len)
//{
//    uchar *buf=new uchar[len+8];
//    buf[0]=0xD2;
//    buf[1]=0x0B;
//    buf[2]=0x00;//发送端口号
//    buf[3]=canID&0xff;
//    buf[4]=canID>>8&0xff;
//    buf[5]=canID>>16&0xff;
//    buf[6]=canID>>24&0xff;

//    if(len>0)
//        memcpy(buf+7,buff,len);
//    buf[len+7]=0;

//    for (short i=1;i<len+7;i++)
//        buf[len+7]+=buf[i];

//    QByteArray array((char *)buf,len+8);
//    senSerialDataByCom(array,buf[2]);

//    delete[] buf;
//    return 0;
//}
int ComManager::sendRecordCanData(uint canID, uchar *buff, unsigned char len)
{
    QByteArray array;
    //array.reserve(len + 8);

    // 依次追加各个字段（append 自动管理下标）
    array.append(static_cast<char>(0xD2));
    //array.append(static_cast<char>(0x0B));
    array.append(static_cast<char>(0x00)); // 发送端口号

    // 将 canID 按小端字节顺序追加（减少手动角标操作）
    array.append(static_cast<char>(canID & 0xFF));
    array.append(static_cast<char>((canID >> 8) & 0xFF));
    array.append(static_cast<char>((canID >> 16) & 0xFF));
    array.append(static_cast<char>((canID >> 24) & 0xFF));

    // 追加数据缓冲区
    if (len > 0)
        array.append(reinterpret_cast<const char*>(buff), len);

    // 预先追加一个占位字节，用于存放校验和
    array.append('\0');

    // 计算校验和：累加从第二个字节开始到校验位之前的所有字节
    // 注意：使用 QByteArray 的 constBegin() 与 constEnd() 返回的迭代器
    uchar checksum = static_cast<uchar>(std::accumulate(
        array.constBegin() + 1,       // 从下标1开始
        array.constEnd() - 1,         // 到倒数第二个字节（即最后一位校验位之前）
        0,
        [](int sum, char c) {
            return sum + static_cast<uchar>(c);
        }
    ));

    // 设置校验和到最后一个字节（append 占位的那一位）
    array[array.size() - 1] = static_cast<char>(checksum);

    // 通过数组第三个字节作为端口号发送数据
    senSerialDataByCom(array, static_cast<uchar>(array.at(1)));

    return 0;
}

int ComManager::sendCanData(uint canID,uchar *buff,unsigned char len)
{
    uchar *buf=new uchar[len+8];
    buf[0]=0x09;
    buf[1]=len+8;
    buf[2]=canID&0xff;
    buf[3]=canID>>8&0xff;
    buf[4]=canID>>16&0xff;
    buf[5]=canID>>24&0xff;
    if(len>0)
        memcpy(buf+6,buff,len);
    buf[len+6]=0;
    buf[len+7]=0x0D;
    for (short i=1;i<len+6;i++)
        buf[len+6]^=buf[i];

    QByteArray array((char *)buf,len+8);
    sendSerialData(array);

    delete[] buf;
    return 0;
}

void ComManager::senSerialDataByCom(QByteArray array,int comIndex)
{
    qDebug()<<"senSerialDataByCom port:"<<comIndex<<"data:"<<array.toHex();
    if(comIndex==0)
    {
        if(serialCom1 && serialCom1->isOpen)
            serialCom1->sendCanMegSigHandle(array);

    }
    else if(comIndex==1)
    {
        if(serialCom2 && serialCom2->isOpen)
            serialCom2->sendCanMegSigHandle(array);
    }
    else if(comIndex==2)
    {
        if(serialCom3 && serialCom3->isOpen)
            serialCom3->sendCanMegSigHandle(array);
    }
}

int ComManager::sendSerialData(QByteArray array)
{
    emit sendCanMegSig(array);
    return 0;
}


//发送应答
//参数  reciveCode  0xE0 炮长   0xE2 通信控制单元   0xE9   系统指挥啊
//参数  cmdCode     应答控制字
//参数  data        数据
//参数  len         数据长度
int ComManager::sendData2Command(unsigned char reciveCode,unsigned char cmdcode,unsigned char *data,int len)
{
    Q_ASSERT(myNetComInterface);
    quint16 SelfAddrCode=iniSettings::Instance()->getSelfAttribute();
    //发送数据 给目的目标
    unsigned char *buff=new unsigned char[len+8]; //数据长度加上 头尾 数据长度 校验位 发站码 收站码 命令字

    buff[0]=0xEB;
    buff[1]=0x90;
    buff[2]=(len+6)&0xFF;
    buff[3]=((len+6)>>8)&0xFF;
    buff[4]=reciveCode;
    buff[5]=SelfAddrCode;
    buff[6]=cmdcode;
    memcpy(buff+7,data,len);
    buff[len+7]=0;
    for(int i=2;i<len+7;i++)
        buff[len+7]+= buff[i];
    buff[len+7]=~buff[len+7]+1;
    int ret=myNetComInterface->sendData(buff,len+8,netSocketIP,netSocketPort);
    QByteArray temarry((char*)buff,len+8);
    qDebug()<<ret <<"net datalen 1:"<<temarry.toHex();

    delete[] buff;
    return ret;
}

//
//发送应答
//参数  reciveCode  0xE0 炮长   0xE2 通信控制单元   0xE9   系统指挥啊
//参数  cmdCode1     应答控制字
//参数  cmdCode2     应答控制字
//参数  data        数据
//参数  len         数据长度
int ComManager::sendData2Command(unsigned char reciveCode,unsigned char cmdcode1,unsigned char cmdcode2,unsigned char *data,int len,
                                 QString ip, int port)
{
    quint16 SelfAddrCode=iniSettings::Instance()->getSelfAttribute();
    //发送数据 给目的目标
    unsigned char *buff=new unsigned char[len+9]; //数据长度加上 头尾 数据长度 校验位 发站码 收站码 命令字

    buff[0]=0xEB;
    buff[1]=0x90;
    buff[2]=(len+7)&0xFF;
    buff[3]=((len+7)>>8)&0xFF;
    buff[4]=reciveCode;
    buff[5]=SelfAddrCode;
    buff[6]=cmdcode1;
    buff[7]=cmdcode2;
    memcpy(buff+7,data,len);
    buff[len+8]=0;
    for(int i=2;i<len+8;i++)
        buff[len+8]+= buff[i];
    buff[len+8]=~buff[len+8]+1;
    //发送数据
    int ret=myNetComInterface->sendData(buff,len+9,ip,port);
    //QByteArray temarry((char *)buff);
    QByteArray temarry((char*)buff,len+9);
    qDebug()<<ret <<"net datalen 2:"<<temarry.toHex();
    delete[] buff;
    return ret;
}
//
//发送应答
//参数  reciveCode  0xE0 炮长   0xE2 通信控制单元   0xE9   系统指挥啊
//参数  cmdCode1     应答控制字
//参数  cmdCode2     应答控制字
//参数  data        数据
//参数  len         数据长度
int ComManager::sendNetData( QByteArray array, QString ip, int port)
{
    //发送数据 给目的目标
    int len=array.length();
    if(len<4)
        return -1;
    unsigned char *buff=new unsigned char[len+8]; //数据长度加上 头尾 数据长度 校验位 发站码 收站码 命令字
    buff[0]=0xAA;
    buff[1]=0x55;
    buff[2]=0xEB;
    buff[3]=0x90;
    buff[4]=(len+2)&0xFF;
    buff[5]=((len+2)>>8)&0xFF;
    if((array[1]&0xFF)==0x5A)
        buff[6]=0xA2;
    else
        buff[6]=0xA1;
    memcpy(buff+7,array,len);
    buff[len+7]=0;
    for(int i=6;i<len+7;i++)
        buff[len+7]+= buff[i];
    buff[len+7]=buff[len+7]&0xFF;
    //发送数据
    int ret=myNetComInterface->sendData(buff,len+8,ip,port);
    QByteArray temarry((char*)buff,len+8);
    qDebug()<<ret <<"send net datalen---:"<<temarry.toHex();
    delete[] buff;
    return ret;
}

int ComManager::sendNetDataDirectly( QByteArray array, QString ip, int port)
{
    int ret=myNetComInterface->sendData((unsigned char *)array.data(),array.size(),ip,port);
    qDebug()<<ret <<"send net datalen---:"<<ip<<port<<array.toHex();
    return ret;
}
