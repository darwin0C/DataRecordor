#include "qmycom.h"
#include  <QThread>
#include  <QDebug>
#include "MsgSignals.h"
#include <QDateTime>
//const int MinPacketLength = 26;

QMyCom::QMyCom(QObject *parent):
    QThread(parent)
{

    mySeriCom=new QSerialPort();
    connect(mySeriCom, &QSerialPort::readyRead, this, &QMyCom::reciveComData);
    isOpen=false;
    myComRxBuff=new QTCQueue(10240);
    //    connect(&timer,&QTimer::timeout,this,[this](){revDataCount=0;});
    //    timer.start(1000*5);
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
        comDataHandle();
    }
}

void QMyCom::run()
{
    //    while(isOpen)
    //    {
    //        comDataHandle();
    //        msleep(1);
    //    }
}
//void QMyCom::comDataHandle()
//{
//    // -----------------------------
//    // 1. 先把所有可读数据一次性 peek 到本地 QByteArray
//    // -----------------------------
//    int available = myComRxBuff->InUseCount();
//    // 定义“最小整帧长度”，与 SerialDataRev 等宽
//    const int MinPacketLength = sizeof(SerialDataRev);
//    if (available < MinPacketLength) {
//        // 缓冲区中连一个完整帧都不足，直接返回
//        return;
//    }

//    QByteArray buf(available, Qt::Uninitialized);
//    myComRxBuff->Peek(buf.data(), available);

//    // -----------------------------
//    // 2. 在 buf 中循环查找并处理“帧”
//    // -----------------------------
//    int pos = 0;
//    // 帧头 / 第二字节常量
//    const char FRAME_HEAD   = char(0xC1);
//    const char FRAME_SECOND = char(0x0A);
//    //qDebug() << "myComRxBuff->Peek"<<buf.toHex();
//    while (true) {
//        // 2.1 保证从当前 pos 开始到 buf 末尾至少有 MinPacketLength 字节，否则不够一帧，退出循环
//        if (pos + MinPacketLength > buf.size()) {
//            // qDebug() << "pos + MinPacketLength > buf.size()";
//            break;
//        }

//        // 2.2 从 pos 起搜索下一个可能的帧头
//        int idx = buf.indexOf(FRAME_HEAD, pos);
//        if (idx < 0) {
//            // 后面没有 0xC1，退出
//            //qDebug() << "no 0xC1";
//            break;
//        }

//        // 2.3 如果 idx 后面连第二字节都读不到，说明数据不够，留给下一轮续包
//        if (idx + 1 >= buf.size()) {
//            //qDebug() << "idx + 1 >= buf.size()";
//            break;
//        }

//        // 2.4 第二字节不对，则说明这不是合法帧头，跳过这一个位置，继续找
//        if (buf.at(idx + 1) != FRAME_SECOND) {
//            pos = idx + 1;
//            qDebug() << "error,buf.at(idx + 1) != FRAME_SECOND"<<buf.at(idx + 1);
//            continue;
//        }

//        // 2.5 到这里说明：buf[idx] == 0xC1，buf[idx+1] == 0x0A，有可能是一帧。
//        //      还要再确认是否能读到完整 MinPacketLength 字节：
//        if (idx + MinPacketLength > buf.size()) {
//            // 从 idx 开始不够一个完整帧，等下次再续包
//            //qDebug() << "idx + MinPacketLength > buf.size()";
//            break;
//        }

//        // 2.6 拿到这一帧的数据指针，开始校验
//        uchar* frame = reinterpret_cast< uchar*>(buf.data() + idx);
//        if (!andCheck(frame, MinPacketLength)) {
//            // 校验失败，跳过这个 idx，再往后继续搜索
//            qDebug() << "error ,check fail，jump idx =" << idx
//                     << "，data：" << buf.mid(idx, MinPacketLength).toHex();
//            pos = idx + 1;
//            continue;
//        }

//        // 2.7 校验通过，把这一帧 memcpy 出来，然后发信号
//        SerialDataRev stFromOPCData;
//        memcpy(&stFromOPCData, frame, MinPacketLength);

//        CanData CanDataRev;
//        memcpy(&CanDataRev, &stFromOPCData.candata, sizeof(CanData));

//        emit MsgSignals::getInstance()->serialDataSig(stFromOPCData);
//        emit MsgSignals::getInstance()->canDataSig(CanDataRev);
//        //qDebug() << "emit MsgSignals::getInstance()->serialDataSig";
//        // 2.8 消费这一帧：把 pos 移到 idx + MinPacketLength，继续循环
//        pos = idx + MinPacketLength;
//    }

//    // -----------------------------
//    // 3. 从底层缓冲区一次性移除已处理的 pos 个字节
//    // -----------------------------
//    if (pos > 0) {
//        myComRxBuff->MoveReadP(pos);
//    }
//}

void QMyCom::comDataHandle()
{
    unsigned char tembuff[64]={0};
    const int MinPacketLength = sizeof(SerialDataRev);
    while(myComRxBuff->InUseCount()>=MinPacketLength )
    {

        myComRxBuff->Peek(tembuff,MinPacketLength);
        unsigned char head=tembuff[0];
        unsigned char flag=tembuff[1];
        if(head!=0XC1 || flag!=0x0A)
        {
            qDebug() << "head or flag error"<<head<<flag;
            myComRxBuff->MoveReadP(1);
            continue;
        }
        if (andCheck(tembuff,MinPacketLength)) //判断收到的数据是否正确
        {
            SerialDataRev stFromOPCData;
            CanData CanDataRev;
            myComRxBuff->Get(&stFromOPCData,MinPacketLength);
            memcpy(&CanDataRev,&stFromOPCData.candata,sizeof(CanData));
            //qDebug() << "emit serialDataSig==========================";
            emit MsgSignals::getInstance()->serialDataSig(stFromOPCData);
            emit MsgSignals::getInstance()->canDataSig(CanDataRev);
        }else
        {
            qDebug() << "check error=========================="<<QByteArray((char *)tembuff,MinPacketLength).toHex();
            myComRxBuff->MoveReadP(1);
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

