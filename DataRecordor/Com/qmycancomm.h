#ifndef QMYCANCOMM_H
#define QMYCANCOMM_H

#include <QObject>
#include <QTimer>
//#include <QSerialPort>

//#include "qtcqueue.h"
//class QSerialPort;

#define  T750    750 
#define  T1250   1250  
#define  T1050   1050  
// //#define  T4    750
//发送CTS时每次发送的包数
#define  TMCTSSendPackets 5

//#define  MaxRecLinkNum  7
#pragma pack(1)  //内存1字节对齐
enum LongDataSendFlag
{
     LongDataSend_RTS    =1,        //发送了申请
     LongDataSend_CTS    =2,	    //收到了对方应答，建立了连接
     LongDataSend_DT     =3,	    //发送了数据
     LongDataSend_End    =4,	    //数据全收到断开连接
     //LongDataSend_Abort=5,	    //放弃连接
     LongDataSend_DTDelay=6,	    //接收数据延时
     LongData_DTError    =7,        //接收数据有误
     LongData_DTNextCTS  =8,	    //一个CTS数据接收完成准备按一次的CTS的发送和接收
	// LongDataBroadCast_RTS=9,     //接收到广播公告
	// LongDataBroadCast_DT=10,     //接收到广播数据
     LongDataSend_NULL   =0,        //初始状态，没有任何状态
};
//数据
typedef struct  
{  
	unsigned char    datasendflag;            //数据标识
	unsigned int     addr;					  //地址
	unsigned char    *data;                  //数据缓冲区
	unsigned char    pgn[3];                 //数据PNG
	unsigned short	 cnt;                     //计数器
	unsigned short   datalen;				  //数据长度
	unsigned char    packetsnum;			  //数据包个数
	unsigned char    sendpacketstartnum;    //需要发送的开始数据包
	unsigned char    sendpacketsnum;       //需要发送的包数
	unsigned char    havesendrecievepacketnum;     //已发送或则已接收的包数
}P2PCanData;
#pragma pack()  //取消内存对齐

//数据
// typedef struct  
// { 
// 
// }BroadcastCanData;

class QmyCanComm : public QObject
{
	Q_OBJECT

public:
	 //QmyCanComm(QObject *parent);
     QmyCanComm(QObject *parent= nullptr);
	~QmyCanComm();
     static QmyCanComm  *myInuCanCom;
	//发送数据
	int  sendData(uint canID,uchar *buff,unsigned char len);
	//发送RTS
	int sendTM_RTS(uint canID,unsigned short allbytes,unsigned char packets,unsigned char PGN[3],unsigned char *senddata);
	//   接收的数据处理
	void recieveLinkDataHandle(unsigned int canID,uchar *buff);
	//连接建立后发送数据包//长包发送
	int  sendTM_DT(uint canID,unsigned char *buff,unsigned char startnum);
	//连接建立后收到CTS
	void recieveResponsorTM_CTS(uchar *buff);
	//接收到RTS建立连接请求 发送CTS
	void recieveTM_RTS(unsigned int canID, uchar *buff);
	//当接收到RTS后发送给请求段CTS
	int sendTM_CTS(unsigned int canID,unsigned char sendpacketsnum,unsigned char sendpacketstartnum);
	int sendTM_EndOfMsg(unsigned int canID);
	unsigned int changRecIDToSendID(unsigned int canid);

	void recieveTM_DT(uchar *buff);
	//拆包

	//广播数据处理
	void recieveBroadcastTM_RTS(unsigned int canID, uchar *buff);
	void recieveBroadcastTM_DT(unsigned char *buff);

    static QmyCanComm *instance();
private:
	QTimer     *myTimer;
	//QTimer *myTimer;
	P2PCanData *myCanSendData;
	P2PCanData *myCanRecData;//[MaxRecLinkNum];
	P2PCanData *myCanBroadCastRecData;
   // QSerialPort *mycom;
	//unsigned char recLinkNum;
private slots:
	void timerhandle();
signals:
	void CanDataReady(unsigned char *pgn,unsigned char *data,unsigned short datalen,bool bbroadcast=false);

};

#endif // QMYCANCOMM_H
