#ifndef QMYNETCOM_H
#define QMYNETCOM_H

#include <QObject>
#include <QUdpSocket>
#include "qtcqueue.h"
#include <QThread>
#include "data.h"

//用于侦察车侦察终端与指挥之间的通信协议
#define   SelfAddrCode    0xE5
#define   CommandAdrrCode 0xE0
#define   ComUnitAdrrCode 0xE2
#define   systemAdrrCode  0xE9
#define   GPSAdrrCode 0xE4
#define   NavigationAdrrCode 0xEF
#define   NetDataPacketLength   6
#pragma pack(1)
typedef struct
{
    unsigned char   Head[4];      //0xAA55EB90
    unsigned char   Length[2];    //数据包长度
    unsigned char   flag;         //传输标识
    unsigned int    dataid;       //id
    unsigned char   data[8];      //8字节的数据
    unsigned char   Xor;          //异或校验

}Net2CANData;

typedef struct
{
    unsigned char   Head[2];      //0xEB90
    unsigned char   Length[2];    //数据包长度
    unsigned char   flag;         //传输标识
    unsigned int    dataid;       //id
    unsigned char   data[8];      //8字节的数据
    unsigned char   Xor;          //异或校验

}ChexiaWired2CANData;

#pragma pack()
class QMyNetCom : public QThread
{
    Q_OBJECT

public:
    explicit QMyNetCom(QObject *parent = 0);
    ~QMyNetCom();
    QUdpSocket   *myCommandUdp;

    QTCQueue     *myNetRx;              //网口接收缓冲区
    bool         bcommandudpopen;
    void initSocket(uint port);
    void netDataHandle();
    int Verify_Sum_Complement(unsigned char *sBytes, int iBytesCount);

    void initSocket(QString ip, uint port);
    int sendData(unsigned char *data, int len, QString ip, int port);
    void commandDataHandle(char *buff,ushort dlen);

    bool joinGroup(QString udpGroup);
signals:
    void canDataSig(CanData);
    void sendQByteArraySig(QByteArray);
public slots:

private slots:
    void udpCommandDataRecive();
private:
    void run();
    int Verify_Sum(unsigned char *sBytes, int iBytesCount);
    int Verify_Sum2(unsigned char *sBytes, int iBytesCount);
    int Verify_SumLow8(unsigned char *sBytes, int iBytesCount);
};

#endif // QMYNETCOM_H
