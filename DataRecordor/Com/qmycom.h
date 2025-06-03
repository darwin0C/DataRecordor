#ifndef QMYCOM_H
#define QMYCOM_H

#include <QObject>
#include <QSerialPort>
#include "qtcqueue.h"
#include <QTimer>
#include <QThread>
#include "data.h"


enum SerialComSource{
    ComSource_default   =0,
    ComSource_beidou    =1,
    ComSource_keyBoard  =2,
};

#pragma pack(1)
typedef struct
{
    unsigned char   Head;                  //0x09
    unsigned char   Length;                //数据包长度
    unsigned int    dataid;
    unsigned char   data[8];               //8字节的数据
    unsigned char   Xor;                   //异或校验
    unsigned char   Tail;                  //帧尾0x0D
}Can2SeriData;
#pragma pack()

class QMyCom : public QThread
{
    Q_OBJECT
    int revDataCount=0;
    //QTimer timer;
public:
    explicit QMyCom(QObject *parent = 0);
    ~QMyCom();
    QSerialPort *mySeriCom;
    bool    isOpen;
    QTCQueue *myComRxBuff;
    bool initComInterface(const QString &port, int baund);
    bool isComInterfaceOpen();
    //    int sendData(uint canID, uchar *buff, unsigned char len);
    //    int sendData(char *buff, int len);
private:
    int  commFrameXorNohead(unsigned char *pBuf, unsigned char cFrameHead, unsigned char cFrameTail, unsigned int FrameSize);
    void run();
    bool andCheck(unsigned char *pBuf, unsigned int FrameSize);

    void parseFrames();
public slots:
    void closeComInterface();
    void comDataHandle();
    void sendCanMegSigHandle(QByteArray array);
    void reciveComData();

};

#endif // QMYCOM_H
