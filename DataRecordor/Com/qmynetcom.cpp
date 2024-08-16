#include "qmynetcom.h"
#include  <QThread>

#define DataHead1   0xEB
#define DataHead2   0x90
#define DataHeadAA   0xAA
#define DataHead55   0x55

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
        if(buff[0]==DataHead1&&buff[1]==DataHead2) //判断数据头
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
                //qDebug()<<"check right";
                if(((char)(SelfAddrCode&0xFF))==tem[4])
                {
                    // qDebug()<<"check right...E5";
                    myNetRx->Get(tem,packlen);
                    commandDataHandle(tem,packlen);
                }
                else
                    myNetRx->MoveReadP(1);
            }else
                myNetRx->MoveReadP(1);
            delete[] tem;
        }
        else if(buff[0]==DataHeadAA&&buff[1]==DataHead55 &&buff[2]==DataHead1&&buff[3]==DataHead2)
        {

            len=buff[5]<<8|buff[4];
            packlen=len+6;
            if(packlen>myNetRx->InUseCount()) return ;

            if(packlen>MaxPacketLen/*||(len<5)*/)
            {
                myNetRx->MoveReadP(1);
                return;
            }
            char *tem=new char[packlen];
            myNetRx->Peek(tem,packlen);
            if(buff[6]==0xA1 || buff[6]==0xA2)
            {
                Net2CANData stFromOPCData;
                CanData CanDataRev;
                if(0==Verify_Sum((unsigned char *)tem,packlen))
                {
                    QByteArray array((char *)tem,packlen);
                    myNetRx->Get(&stFromOPCData,packlen);
                    CanDataRev.dataid=stFromOPCData.dataid;
                    memcpy(CanDataRev.data,stFromOPCData.data,8);
                    CanDataRev.len=8;
                    if (((((stFromOPCData.dataid>>8)&0xff)==0x50))||//短包数据
                            (stFromOPCData.dataid>>16&0xff)>=0xF1) //长包广播数据
                    {
                        emit canDataSig(CanDataRev);
                        QByteArray array((char *)&CanDataRev.dataid,4);
                        QByteArray array2((char *)&CanDataRev.data,8);
                        //qDebug()<<"rev UDP Data 2:"<<array.toHex()<<array2.toHex();
                    }
                    else
                    {
                        //emit canDataSig(CanDataRev);
                        QByteArray array((char *)&CanDataRev.dataid,4);
                        QByteArray array2((char *)&CanDataRev.data,8);
                        //qDebug()<<"rev UDP Data 2:"<<array.toHex()<<array2.toHex();
                    }
                }else
                    myNetRx->MoveReadP(1);
                delete[] tem;
            }
            else
            {
                //packlen+=1;
                packlen=len+7;
                if(packlen>myNetRx->InUseCount()) return ;
                char *tem=new char[packlen];
                myNetRx->Peek(tem,packlen);
                QByteArray arrayTemp((char *)(tem),packlen);
                qDebug()<<"remote ctrl:"<<arrayTemp.toHex();
                if(tem[packlen-1]==(char)0xEE/*0==Verify_Sum2((unsigned char *)tem,packlen)*/)
                {
                    myNetRx->Get(tem,packlen);
                    QByteArray array((char *)(tem+6),len);
                    emit sendQByteArraySig(array);
                }else
                    myNetRx->MoveReadP(1);
                delete[] tem;
            }
        }
        else if(buff[0]==0x7F && buff[1]==0x24)//雷达导引数据
        {
            if(myNetRx->InUseCount()<10) return ;
            uchar buffTemp[10]={0};
            myNetRx->Peek(buffTemp,10);
            packlen=buffTemp[9]<<8|buffTemp[8];
            if(packlen>myNetRx->InUseCount())
            {
                return;
            }
            char *tem=new char[packlen];
            myNetRx->Peek(tem,packlen);
            unsigned char checksum=tem[packlen-2];
            if(checksum==Verify_SumLow8((unsigned char *)tem,packlen-2))
            {
                myNetRx->Get(tem,packlen);
                QByteArray array((char *)(tem),packlen);
                emit sendQByteArraySig(array);
                qDebug()<<"receive Radar data:"<<array.toHex();
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
    //buff 0-5  // 0-1 0xEB90  2-3 len  4 收站码0xE5  5发站码
    QByteArray temData;
    unsigned char sendCode=buff[5]; //发站码
    unsigned char cmdCode=buff[6]; //命令字1

}

