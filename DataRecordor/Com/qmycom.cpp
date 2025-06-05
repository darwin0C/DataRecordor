#include "qmycom.h"
#include  <QThread>
#include  <QDebug>
#include "MsgSignals.h"
#include <QDateTime>
#include <QQueue>
#include "canmsgreader.h"
extern QQueue<SerialDataRev> SerialDataQune;
extern QMutex gMutex;
QMyCom::QMyCom(int index,QObject *parent):
    QObject(parent)
{
    comIndex=index;
    m_isOpen=false;
    m_rxBuf=new QTCQueue(1*1024*1024);
    MinPacketLength = sizeof(SerialDataRev);
}
QMyCom::~QMyCom()
{
    closePort();
    delete mySeriCom;
    delete m_rxBuf;
}
//初始化通信口
bool QMyCom::initComInterface(const QString &port, int baud)
{

    mySeriCom=new QSerialPort(this);
    connect(mySeriCom, &QSerialPort::readyRead, this, &QMyCom::reciveComData/*, Qt::DirectConnection*/);

    if (m_isOpen) closePort();

    mySeriCom->setPortName(port);
    mySeriCom->setBaudRate(baud);
    mySeriCom->setStopBits(QSerialPort::OneStop);
    mySeriCom->setParity(QSerialPort::NoParity);
    mySeriCom->setDataBits(QSerialPort::Data8);

    m_isOpen = mySeriCom->open(QIODevice::ReadWrite);
    qDebug() << "[Serial" << comIndex << "] open"
             << (m_isOpen ? "OK" : mySeriCom->errorString())
             << "in thread" << QThread::currentThread();
    /* ---------- 定时器同样在子线程 new ---------- */
    timer = new QTimer(this);
    connect(timer,&QTimer::timeout,this,&QMyCom::onTimeHandle,Qt::DirectConnection);
    timer->start(1000);
    return m_isOpen;
}
void QMyCom::closePort()
{
    if (m_isOpen) mySeriCom->close();
    m_isOpen = false;
}

void QMyCom::onTimeHandle()
{
    revDataCount++;
    if(revDataCount>10)
    {
        comReady=true;
        qDebug()<<"comReady"<<comIndex;
        timer->stop();
    }
}

//关闭
void QMyCom::closeComInterface()
{
    if(mySeriCom==NULL) return ;
    if(m_isOpen)
    {
        mySeriCom->close();
        m_isOpen=false;
    }
}

bool QMyCom::isComInterfaceOpen()
{
    return m_isOpen;
}


void QMyCom::reciveComData()
{
    QByteArray tempData = mySeriCom->readAll();
    //qDebug()<<"com rev: "<<tempData.toHex();

    revDataCount=0;
    if (!tempData.isEmpty()) {
        m_rxBuf->Add(tempData.data(), tempData.size());
        comDataHandle();
    }
}

//void QMyCom::run()
//{
//    while(m_isOpen)
//    {
//        //if(comReady)
//        comDataHandle();
//        msleep(1);
//    }
//}

void QMyCom::comDataHandle()
{


    unsigned char frame[MinPacketLength];

    while(m_rxBuf->InUseCount()>=MinPacketLength )
    {

        m_rxBuf->Peek(frame,MinPacketLength);

        if (frame[0] != 0xC1 || frame[1] != 0x0A) {
            // 快速同步：一次性跳到下一个 0xC1
            int skip = m_rxBuf->IndexOf(0xC1);
            m_rxBuf->MoveReadP(skip > 0 ? skip : 1);
            //m_rxBuf->MoveReadP(1);
            qDebug() << "Head or Flag error"<<frame[0]<<frame[1];
            continue;
        }

        if (!andCheck(frame, MinPacketLength)) {
            m_rxBuf->MoveReadP(1);
            qDebug() << "check error=========================="<<QByteArray((char *)frame,MinPacketLength).toHex();
            continue;
        }
        SerialDataRev stFromOPCData;
        m_rxBuf->Get(&stFromOPCData,MinPacketLength);
        gMutex.lock();
        SerialDataQune.enqueue(stFromOPCData);
        gMutex.unlock();
        //emit MsgSignals::getInstance()->serialDataSig(stFromOPCData);
        //qDebug() << "emit serialDataSig==========================";
        uint canid=stFromOPCData.candata.dataid;
        if(CanMsgReader::Instance()->CANIDSet.contains(canid)||
                canid>>16==0x1CEC ||canid>>16==0x1CEB)
        {
            CanData CanDataRev;
            memcpy(&CanDataRev,&stFromOPCData.candata,sizeof(CanData));
            emit MsgSignals::getInstance()->canDataSig(CanDataRev);
        }

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

