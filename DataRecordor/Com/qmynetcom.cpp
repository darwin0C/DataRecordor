#include "qmynetcom.h"
#include  <QThread>
#include "MsgSignals.h"

#define DataHeadEB   0xEB
#define DataHead90   0x90

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
    while(myNetRx->InUseCount()>NetDataPacketLength)  //最短包的长度
    {
        uchar buff[8]={0};
        ushort packlen=0;//接收一包数据长度
        ushort len=0;  //数据包里面的数据长度
        myNetRx->Peek(buff,8);
        if(buff[0]==DataHeadEB&&buff[1]==DataHead90) //判断数据头
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
                    commandDataHandle(tem,packlen);
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


void QMyNetCom::commandDataHandle(char *buff,ushort dlen)
{
    //    //buff 0-5  // 0-1 0xEB90  2-3 len  4 收站码0xE5  5发站码
    //    QByteArray temData;
    //    unsigned char sendCode=buff[5]; //发站码
    //    unsigned char cmdCode=buff[6]; //命令字1

    QByteArray byteArray(buff,dlen);
    int start = 6;  // 第7位 命令字开始
    int length = byteArray.size() - 7;  // 计算长度

    QByteArray result = byteArray.mid(start, length);
    emit MsgSignals::getInstance()->commandDataSig(result);
    qDebug() << result;  //
}

