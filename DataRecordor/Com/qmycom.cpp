#include "qmycom.h"
#include  <QThread>
#include  <QDebug>
#include "MsgSignals.h"
#include <QDateTime>
const int MinPacketLength = 26;

QMyCom::QMyCom(QObject *parent):
    QThread(parent)
{

    mySeriCom=new QSerialPort(this);
    connect(mySeriCom, &QSerialPort::readyRead, this, &QMyCom::reciveComData);
    isOpen=false;
    myComRxBuff=new QTCQueue(10240);
}
QMyCom::~QMyCom()
{
    if (isOpen) {
        mySeriCom->close();
    }
    delete mySeriCom;
    delete myComRxBuff;
}
//初始化通信口
bool QMyCom::initComInterface(const QString &port, int baud)
{
    if (isOpen) {
        mySeriCom->close();
        isOpen = false;
    }

    mySeriCom->setPortName(port);
    mySeriCom->setBaudRate(baud);
    mySeriCom->setStopBits(QSerialPort::OneStop);
    mySeriCom->setParity(QSerialPort::NoParity);
    mySeriCom->setDataBits(QSerialPort::Data8);

    isOpen = mySeriCom->open(QIODevice::ReadWrite);
    if (!isOpen) {
        qDebug() << mySeriCom->errorString() << "Serial port error";
    } else {
        qDebug() << "Serial port opened successfully";
    }

    return isOpen;
}

//关闭
void QMyCom::closeComInterface()
{
    if(mySeriCom==NULL) return ;
    if(isOpen)
    {
        mySeriCom->close();
        isOpen=false;
    }
}

bool QMyCom::isComInterfaceOpen()
{
    return isOpen;
}


void QMyCom::reciveComData()
{
    QByteArray tempData = mySeriCom->readAll();
    if (!tempData.isEmpty()) {
        myComRxBuff->Add(tempData.data(), tempData.size());
        qDebug()<<"data rev:"<<tempData.toHex();
    }
}

void QMyCom::run()
{
    while(isOpen)
    {
        comDataHandle();
        msleep(10);
    }
}
void QMyCom::comDataHandle()
{
    unsigned char tembuff[64]={0};

    while(myComRxBuff->InUseCount()>=MinPacketLength )
    {
        SerialDataRev stFromOPCData;
        CanData CanDataRev;
        myComRxBuff->Peek(tembuff,MinPacketLength);
        unsigned char head=tembuff[0];
        unsigned char flag=tembuff[1];
        if(head!=0XC1 || flag!=0x0A)
        {
            myComRxBuff->MoveReadP(1);
            continue;
        }

        if (andCheck(tembuff,MinPacketLength)) //判断收到的数据是否正确
        {
            myComRxBuff->Get(&stFromOPCData,MinPacketLength);
            memcpy(&CanDataRev,&stFromOPCData.candata,sizeof(CanData));
            //qDebug() << "emit serialDataSig==========================";
            emit MsgSignals::getInstance()->serialDataSig(stFromOPCData);
            emit MsgSignals::getInstance()->canDataSig(CanDataRev);
        }else
            myComRxBuff->MoveReadP(1);
    }
}

bool QMyCom::andCheck(unsigned char *pBuf, unsigned int FrameSize) {
    if (FrameSize == 0) {
        return false; // 如果帧大小为 0，直接返回 false
    }
    unsigned char sum = 0;
    // 对所有字节（除第一个最后一个字节外）进行求和
    for (unsigned int i = 1; i < FrameSize - 1; ++i) {
        sum += pBuf[i];
    }
    // 取低8位
    sum = sum & 0xFF;

    // 比较计算的校验和与最后一个字节（校验位）
    return (sum == pBuf[FrameSize - 1]);
}


//---------------------------------------------------------------------------
//	处理串口接收数据, 判断数据帧正确性
//    适用于操控台
//  参数:   pBuf:       数据缓冲区指针
//          cFrameHead: 帧头
//          cFrameTail: 帧尾
//          FrameSize:  帧长度
//  返回:   =0:     接收到正确的帧
//          =-1:    头或者尾不正确
//          =-2:    校验和不正确
//	注意:
//	1.  如果检出正确帧之后,仍然要检索完整个缓冲区,需要在调用此函数时循环处理,此处不处理,只检出第一包正确的帧就返回;
//			在调用程序中移动指针.
int QMyCom::commFrameXorNohead(	unsigned char *pBuf,
                                unsigned char cFrameHead,
                                unsigned char cFrameTail,
                                unsigned int FrameSize  )
{
    unsigned char  VerXor;
    unsigned int  i;
    if((pBuf[0] == cFrameHead )&&(pBuf[FrameSize-1] == cFrameTail ))//尾
    {
        // 异或校验
        for( VerXor=0,i=1;i<FrameSize-1;i++)
            VerXor ^= pBuf[i];
        //
        if( VerXor==0 ) {
            return 0;
        }else{
            return -2;
        }
    }else{
        return -1;
    }
}
void QMyCom::sendCanMegSigHandle(QByteArray array )
{

    //qDebug()<<"sendCanMegSigHandle"<<QDateTime::currentDateTime().toString("HH:mm:ss.zzz")<<array.toHex();

    Q_ASSERT(mySeriCom!=NULL);
    mySeriCom->write(array);
}
//发送函数
//	短包通用传输函数 8字节内容以内的
//int QMyCom::sendData(uint canID,uchar *buff,unsigned char len)
//{
//    Q_ASSERT(mySeriCom!=NULL);
//    //if (mycom==NULL)  return -3;
//    uchar *buf=new uchar[len+8];
//    buf[0]=0x09;
//    buf[1]=len+8;
//    buf[2]=canID&0xff;
//    buf[3]=canID>>8&0xff;
//    buf[4]=canID>>16&0xff;
//    buf[5]=canID>>24&0xff;
//    if(len>0)
//        memcpy(buf+6,buff,len);
//    buf[len+6]=0;
//    buf[len+7]=0x0D;
//    for (short i=1;i<len+6;i++)
//        buf[len+6]^=buf[i];
//    int  a=mySeriCom->write((char *)buf,len+8);
//    delete[] buf;
//    return a;
//}
////	短包通用传输函数 8字节内容以内的
//int QMyCom::sendData(char *buff,int len)
//{
//    Q_ASSERT(mySeriCom!=NULL);
//    return mySeriCom->write(buff,len);
//}

