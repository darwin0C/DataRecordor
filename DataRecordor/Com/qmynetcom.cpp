#include "qmynetcom.h"
#include  <QThread>
#include "MsgSignals.h"
#include "inisettings.h"

#define DataHeadEB   0xEB
#define DataHead90   0x90
#define DataHead48   0x48

#define MaxPacketLen  512
QMyNetCom::QMyNetCom(QObject *parent) :
    QThread(parent)
{
    myCommandUdp=new QUdpSocket(this);
    myNetRx=new QTCQueue(1024*10);              //网口接收缓冲区
    bcommandudpopen=false;
}
QMyNetCom::~QMyNetCom()
{
    bcommandudpopen=false;
    wait();
    delete myCommandUdp;
    delete myNetRx;
}
void QMyNetCom::initSocket(uint port)
{
    bcommandudpopen=myCommandUdp->bind(QHostAddress::AnyIPv4,port,QAbstractSocket::ShareAddress);//,QAbstractSocket::ShareAddress|QAbstractSocket::ReuseAddressHint);
    connect(myCommandUdp,SIGNAL(readyRead()),this,SLOT(udpCommandDataRecive()));
    qDebug()<<"udpopen"<<bcommandudpopen;
}
void QMyNetCom::initSocket(QString ip,uint port)
{
    bcommandudpopen=myCommandUdp->bind(QHostAddress(ip),port,QAbstractSocket::ShareAddress);//,QAbstractSocket::ShareAddress|QAbstractSocket::ReuseAddressHint);
    connect(myCommandUdp,SIGNAL(readyRead()),this,SLOT(udpCommandDataRecive()));
}
bool QMyNetCom::joinGroup(QString udpGroup)
{
    bool res=myCommandUdp->joinMulticastGroup(QHostAddress(udpGroup));
    qDebug()<<"join Group :"<<res;
    return res;
}
void QMyNetCom::udpCommandDataRecive()
{
    if (myCommandUdp->hasPendingDatagrams()) {
        QByteArray data;
        data.clear();
        data.resize(myCommandUdp->pendingDatagramSize());  //4个字节用于存放Ip地址
        QHostAddress sourceip;
        myCommandUdp->readDatagram(data.data(),data.size(),&sourceip);
        myNetRx->Add(data.data(),data.size());
        //qDebug()<<"udp data:"<<data.toHex();
    }
}
void QMyNetCom::run()
{
    while(bcommandudpopen)
    {
        netDataHandle();
        msleep(5);
    }
}
void QMyNetCom::netDataHandle()
{
    quint16 SelfAddrCode=iniSettings::Instance()->getSelfAttribute();
    while(myNetRx->InUseCount()>NetDataPacketLength)  //最短包的长度
    {
        uchar buff[8]={0};
        ushort packlen=0;//接收一包数据长度
        ushort len=0;  //数据包里面的数据长度
        myNetRx->Peek(buff,8);
        if(buff[0]==DataHeadEB && buff[1]==DataHead90) //判断数据头
        {
            //指挥数据处理
            len=buff[3]<<8|buff[2];
            packlen=len+2;
            if(packlen>myNetRx->InUseCount()) return ;
            if(packlen>MaxPacketLen||(len<5))
            {
                myNetRx->MoveReadP(1);
                return;
            }
            char *tem=new char[packlen];
            myNetRx->Peek(tem,packlen);
            if(0==Verify_Sum_Complement((unsigned char *)tem,packlen))
            {
                if(((char)(SelfAddrCode&0xFF))==tem[4])
                {
                    myNetRx->Get(tem,packlen);
                    eb90commandDataHandle(tem,packlen);
                }
                else
                    myNetRx->MoveReadP(1);
            }else
                myNetRx->MoveReadP(1);
            delete[] tem;
        }
        else if(buff[0]==DataHeadEB && buff[1]==DataHead48) //判断数据头 0xEB48数据
        {
            //指挥数据处理
            len=buff[3]<<8|buff[2];
            packlen=len+2;
            if(packlen>myNetRx->InUseCount()) return ;
            if(packlen>MaxPacketLen||(len<5))
            {
                myNetRx->MoveReadP(1);
                return;
            }
            char *tem=new char[packlen];
            myNetRx->Peek(tem,packlen);
            if(0==Verify_Sum_Complement((unsigned char *)tem,packlen))
            {

                ushort addrCode=tem[5]<<8 | tem[4];
                if(((ushort)(SelfAddrCode&0xFFFF))==addrCode)
                {
                    myNetRx->Get(tem,packlen);
                    eb48commandDataHandle(tem,packlen);
                }
                else
                    myNetRx->MoveReadP(1);
            }else
                myNetRx->MoveReadP(1);
            delete[] tem;
        }
        else
            myNetRx->MoveReadP(1);
    }
}
//--------------------------------------------------------------------------
//	检查累加和的补码的校验的正确性
//
//  参数:   sBytes:			被校验的字节串
//			iBytesCount:	被校验字节串的字节数
//	返回:   0:校验正确,1:校验不正确
//  注意:   被校验字节串格式:   (被校验字节... + 1字节校验和)
int QMyNetCom::Verify_Sum_Complement( unsigned char *sBytes,int iBytesCount)
{
    unsigned char ucCheckSum = 0;		//累加和
    int i;
    //校验字节累加
    for( i=2; i<iBytesCount-1; i++ )
        ucCheckSum += *(sBytes+i);
    //求补码
    ucCheckSum=~ucCheckSum+1;
    //比较校验和的补码
    if( ucCheckSum == *(sBytes + iBytesCount -1))
        return 0;	//累加和补码正确
    else
        return 1;	//累加和补码错误
}

int QMyNetCom::Verify_Sum( unsigned char *sBytes,int iBytesCount)
{
    unsigned char ucCheckSum = 0;		//累加和
    int i;
    //校验字节累加
    for( i=6; i<iBytesCount-1; i++ )
        ucCheckSum += *(sBytes+i);
    //取低8位
    ucCheckSum=ucCheckSum&0xFF;
    //比较校验和的补码
    if( ucCheckSum == *(sBytes + iBytesCount -1))
        return 0;	//累加和补码正确
    else
        return 1;	//累加和补码错误
}
int QMyNetCom::Verify_Sum2( unsigned char *sBytes,int iBytesCount)
{
    unsigned char ucCheckSum = 0;		//累加和
    int i;
    //校验字节累加
    for( i=4; i<iBytesCount-2; i++ )
        ucCheckSum += *(sBytes+i);
    //取低8位
    ucCheckSum=ucCheckSum&0xFF;
    //比较校验和的补码
    if( ucCheckSum == *(sBytes + iBytesCount -2))
        return 0;	//累加和补码正确
    else
        return 1;	//累加和补码错误
}
//返回所有数据的累加和低8位
int QMyNetCom::Verify_SumLow8( unsigned char *sBytes,int iBytesCount)
{
    unsigned char ucCheckSum = 0;		//累加和
    //校验字节累加
    for(int i=0; i<iBytesCount; i++ )
        ucCheckSum += *(sBytes+i);
    //取低8位
    ucCheckSum=ucCheckSum&0xFF;
    return ucCheckSum;	//
}
int QMyNetCom::sendData(unsigned char *data,int len, QString ip,int port)
{
    return myCommandUdp->writeDatagram((char*)data,len,QHostAddress(ip),port);
}


void QMyNetCom::eb90commandDataHandle(char *buff,ushort dlen)
{
    if(dlen<7)
        return;
    QByteArray byteArray(buff,dlen);
    int start = 7;  // 第8位 命令参数开始
    int length = byteArray.size() - 8;  // 计算长度
    qint8 sendCode=byteArray[5];
    int cmdCode=byteArray[6];
    QByteArray result = byteArray.mid(start, length);
    emit MsgSignals::getInstance()->commandDataSig(sendCode,cmdCode,result);
    qDebug() << result;  //
}
void QMyNetCom::eb48commandDataHandle(char *buff,ushort dlen)
{
    if(dlen<10)
        return;
    QByteArray byteArray(buff,dlen);
    int start = 10;  // 命令参数开始
    int length = byteArray.size() - 11;  // 计算长度
    quint16 sendCode=byteArray[7]<<8|byteArray[6];//发站码
    quint16 cmdCode=byteArray[9]<<8|byteArray[8];//命令字

    QByteArray result = byteArray.mid(start, length);
    emit MsgSignals::getInstance()->commandDataSig(sendCode,cmdCode,result);
    qDebug() << result;  //
}
