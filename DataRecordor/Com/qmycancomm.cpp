#include "qmycancomm.h"
#include <QDebug>
#include  "qmycom.h"
#include "commanager.h"
QmyCanComm  *QmyCanComm::myInuCanCom=NULL;

QmyCanComm:: QmyCanComm(QObject *parent): QObject(parent)
{
    myTimer=new QTimer(this);
    //发送数据
    myCanSendData=NULL;
    //接收数据
    myCanRecData=NULL;
    myCanBroadCastRecData=NULL;
    connect(myTimer,SIGNAL(timeout()),this,SLOT(timerhandle()));
    //recLinkNum=0;

}
QmyCanComm::~QmyCanComm()
{
    if (myTimer->isActive())
    {
        myTimer->stop();
    }
    if (myCanRecData!=NULL)
    {
        if(myCanRecData->data!=NULL)
        {
            delete[] myCanRecData->data;
            myCanRecData->data=NULL  ;
        }
        delete myCanRecData;
        myCanRecData=NULL;
    }
    if (myCanSendData!=NULL)
    {
        if(myCanSendData->data!=NULL)
        {
            delete[] myCanSendData->data;
            myCanSendData->data=NULL  ;
        }
        delete myCanSendData;
        myCanSendData=NULL;
    }
    if (myCanBroadCastRecData!=NULL)
    {
        if(myCanBroadCastRecData->data!=NULL)
        {
            delete[] myCanBroadCastRecData->data;
            myCanBroadCastRecData->data=NULL  ;
        }
        delete  myCanSendData;
        myCanSendData=NULL;
    }
}
QmyCanComm* QmyCanComm::instance()
{
    if(myInuCanCom==NULL)
        myInuCanCom=new QmyCanComm();
    return myInuCanCom;
}
//	短包通用传输函数 8字节内容以内的
int QmyCanComm::sendData(uint canID,uchar *buff,unsigned char len)
{
    return ComManager::instance()->sendRecordCanData(canID,buff,len);
}
//发送发数rts   //发送数据失败返回-1 已建立连接则不再重发建立
int QmyCanComm::sendTM_RTS(uint canID,unsigned short allbytes,unsigned char packets,unsigned char PGN[3],unsigned char *senddata)
{
    //   if (mycom==NULL)  return -3;
    //如果建立了连接或者正在传输数据
    if (myCanSendData!=NULL)
        return -2;
    myCanSendData=new P2PCanData;
    memset(myCanSendData,0,sizeof(P2PCanData));
    // myCanSendData->data=senddata;
    uchar buff[8];
    buff[0]=16;
    buff[1]=allbytes&0xff;
    buff[2]=(allbytes>>8)&0xff;
    buff[3]=packets;
    buff[4]=0xFF;
    buff[5]=PGN[0];
    buff[6]=PGN[1];
    buff[7]=PGN[2];

    myCanSendData->cnt=0;
    myCanSendData->datasendflag=LongDataSend_RTS;
    myCanSendData->addr=canID;
    myCanSendData->datalen=allbytes;
    myCanSendData->packetsnum=packets;

    myCanSendData->data=new unsigned char[allbytes];
    memcpy(myCanSendData->data,senddata,allbytes);

    myCanSendData->pgn[0]=PGN[0];
    myCanSendData->pgn[1]=PGN[1];
    myCanSendData->pgn[2]=PGN[2];
    int a=sendData(canID,buff,8);

    if (!myTimer->isActive())
        myTimer->start(10);
    return a;
}
//
void QmyCanComm::timerhandle()
{  
    if(myCanSendData!=NULL&&myCanSendData->datasendflag!=LongDataSend_NULL)
    {
        myCanSendData->cnt++;
        if (myCanSendData->datasendflag==LongDataSend_RTS)
        {
            if(myCanSendData->cnt>=(T1250/10))	 //等待CTS超时所有标识清零
            {
                if (myCanSendData!=NULL)
                {
                    if(myCanSendData->data!=NULL)
                    {
                        delete[] myCanSendData->data;
                        myCanSendData->data=NULL  ;
                    }
                    delete	myCanSendData;
                    myCanSendData=NULL;
                }
            }
        }else if(myCanSendData->datasendflag==LongDataSend_DT)	//发送了数据 等待新的CTS或者EndOfMsg
        {
            if (myCanSendData->cnt>=(T1250/10))	 //等待CTS超时所有标识清零
            {
                if (myCanSendData!=NULL)
                {
                    if(myCanSendData->data!=NULL)
                    {
                        delete[] myCanSendData->data;
                        myCanSendData->data=NULL  ;
                    }
                    delete	myCanSendData;
                    myCanSendData=NULL;
                }
            }else if(myCanSendData->cnt%5==0)
            {
                if (myCanSendData->sendpacketsnum-myCanSendData->havesendrecievepacketnum>0)
                {
                    sendTM_DT((myCanSendData->addr|0x00FF0000)&0xFFEBFFFF,(unsigned char *)myCanSendData->data,myCanSendData->sendpacketstartnum+myCanSendData->havesendrecievepacketnum);
                }
            }
        }else if (myCanSendData->datasendflag==LongDataSend_DTDelay) //
        {
            if (myCanSendData->cnt>=((T1250+T1050)/10))	 //接收到数据发送延时回告
            {
                if (myCanSendData!=NULL)
                {
                    if(myCanSendData->data!=NULL)
                    {
                        delete[] myCanSendData->data;
                        myCanSendData->data=NULL  ;
                    }
                    delete	myCanSendData;
                    myCanSendData=NULL;
                }
            }
        }
        else if (myCanSendData->datasendflag==LongDataSend_CTS) //
        {
            if (myCanSendData->cnt>=5)	 //
            {
                //发送一包数
                sendTM_DT((myCanSendData->addr|0x00FF0000)&0xFFEBFFFF,(unsigned char*)myCanSendData->data,myCanSendData->sendpacketstartnum);
            }
        }
    }
    //接收数据的状态判断
    if(myCanRecData!=NULL&&myCanRecData->datasendflag!=LongDataSend_NULL)
    {
        myCanRecData->cnt++;
        if (myCanRecData->datasendflag==LongDataSend_CTS)
        {
            if (myCanRecData->cnt*10>T1250)
            {
                if (myCanRecData!=NULL)
                {
                    if(myCanRecData->data!=NULL)
                    {
                        delete[] myCanRecData->data;
                        myCanRecData->data=NULL  ;
                    }
                    delete	myCanRecData;
                    myCanRecData=NULL;
                }
            }
        }else if (myCanRecData->datasendflag==LongDataSend_DT)
        {
            if (myCanRecData->cnt*10>T750)
            {
                if (myCanRecData!=NULL)
                {
                    if(myCanRecData->data!=NULL)
                    {
                        delete[] myCanRecData->data;
                        myCanRecData->data=NULL  ;
                    }
                    delete	myCanRecData;
                    myCanRecData=NULL;
                }
            }
        }
    }
    //判断广播数据接收状态
    //接收数据的状态判断
    if(myCanBroadCastRecData!=NULL&&myCanBroadCastRecData->datasendflag!=LongDataSend_NULL)
    {
        myCanBroadCastRecData->cnt++;

        if (myCanBroadCastRecData->cnt*10>T750)
        {
            if (myCanBroadCastRecData!=NULL)
            {
                if(myCanBroadCastRecData->data!=NULL)
                {
                    delete[] myCanBroadCastRecData->data;
                    myCanBroadCastRecData->data=NULL  ;
                }
                delete	myCanBroadCastRecData;
                myCanBroadCastRecData=NULL;
            }
        }
    }
}
//接收CTS数据并开始发送数据
void QmyCanComm::recieveLinkDataHandle(unsigned int canID,uchar *buff)
{
    switch (buff[0])
    {
    case 16:     //收到RTS连接申请
    {
        recieveTM_RTS(canID,buff);
        Q_ASSERT(myCanRecData!=NULL);
        unsigned char pack=(myCanRecData->packetsnum>=TMCTSSendPackets)?TMCTSSendPackets:myCanRecData->packetsnum;
        sendTM_CTS(changRecIDToSendID(canID),pack,1);
    }
        break;
    case 17:     //收到CTS连接回应
        recieveResponsorTM_CTS(buff);
        break;
    case 19:     //收到EndOfMsg信号
        if (myCanSendData!=NULL)
        {
            if(myCanSendData->data!=NULL)
            {
                delete[] myCanSendData->data;
                myCanSendData->data=NULL  ;
            }
            delete	myCanSendData;
            myCanSendData=NULL;
        }
        break;
    case 32:     //收到广播公告
        recieveBroadcastTM_RTS(canID,buff);
        break;
    case 255:     //收到中断的
        if (myCanSendData!=NULL)
        {
            if(myCanSendData->data!=NULL)
            {
                delete[] myCanSendData->data;
                myCanSendData->data=NULL  ;
            }
            delete	myCanSendData;
            myCanSendData=NULL;
        }
        break;
    }
}
//===============================================================
//连接建立后发送数据包//长包发送
int  QmyCanComm::sendTM_DT(uint canID,unsigned char *buff,unsigned char startnum)
{
    // Q_ASSERT(myCanSendData!=NULL);
    if (myCanSendData==NULL)  return -3;
    unsigned short len=0;
    unsigned char tembuff[8];
    if (myCanSendData->datasendflag==LongDataSend_CTS||myCanSendData->datasendflag==LongDataSend_DT)	   //发数是在每次收到CTS后开启
    {
        tembuff[0]=startnum;//+myCanSendData->havesendrecievepacketnum;
        if (tembuff[0]==myCanSendData->packetsnum) //最后一包
            len=myCanSendData->datalen-(myCanSendData->packetsnum-1)*7	;
        else if (tembuff[0]<myCanSendData->packetsnum)
            len=7;
        memcpy(tembuff+1,buff+(startnum-1)*7,len);
        int a=sendData(canID,tembuff,len+1);

        myCanSendData->datasendflag=LongDataSend_DT; //发送端处于数据发送状态
        myCanSendData->havesendrecievepacketnum++;
        myCanSendData->cnt=0;

        return a;
    }else
        return -2;
}
//=====================================================================================
//建立连接后收到对方的CTS
void  QmyCanComm::recieveResponsorTM_CTS( uchar *buff)
{
    //Q_ASSERT(myCanSendData!=NULL);
    if (myCanSendData==NULL) return ;

    myCanSendData->cnt=0;
    if (buff[1]==0)   //收到延时请求
    {
        if (myCanSendData->datasendflag==LongDataSend_DT)   //延时求情会在发送数据后才会有用
            myCanSendData->datasendflag=LongDataSend_DTDelay;
    }else
    {
        myCanSendData->sendpacketstartnum=buff[2];
        myCanSendData->sendpacketsnum=buff[1];
        if((myCanSendData->sendpacketstartnum>myCanSendData->packetsnum)
                ||(myCanSendData->sendpacketsnum>myCanSendData->packetsnum))
        {
            if (myCanSendData!=NULL)
            {
                if(myCanSendData->data!=NULL)
                {
                    delete[] myCanSendData->data;
                    myCanSendData->data=NULL  ;
                }

                delete	myCanSendData;
                myCanSendData=NULL;
            }
        }
    }
    myCanSendData->datasendflag=LongDataSend_CTS;
    myCanSendData->cnt=0;
    myCanSendData->havesendrecievepacketnum=0;
}
//===============================================================================
//接收数据处理
//接收到RTS建立连接请求 发送CTS
void QmyCanComm::recieveTM_RTS(unsigned int canID,uchar *buff)
{
    ushort len=buff[1]|buff[2]<<8;
    ushort packlen=0;
    if(len%7==0) packlen=len/7;
    else  packlen=len/7+1;
    qDebug()<<"ReciecveRTS"<<(buff[3]!=packlen);
    if(buff[3]!=packlen) return;
    if (myCanRecData==NULL)
    {
        myCanRecData=new P2PCanData;
        qDebug()<<"new myCanRecData";
    }else
    {
        qDebug()<<"myCanRecData->data"<<myCanRecData->data;
        qDebug()<<"myCanRecData"<<myCanRecData;
        if(myCanRecData->data!=NULL)// &&myCanRecData->datalen!=len)
        {
            delete[] myCanRecData->data;
        }
    }
    memset(myCanRecData,0,sizeof(P2PCanData));
    qDebug()<<myCanRecData->data<<" memset myCanRecData->data";
    myCanRecData->datalen=buff[1]|buff[2]<<8;
    myCanRecData->packetsnum=buff[3];
    myCanRecData->addr=canID;

    myCanRecData->data=new unsigned char[myCanRecData->datalen];
    qDebug()<<myCanRecData->datalen<<"myCanRecData->datalen";
    memset(myCanRecData->data,0,myCanRecData->datalen);
    myCanRecData->pgn[0]=buff[5];
    myCanRecData->pgn[1]=buff[6];
    myCanRecData->pgn[2]=buff[7];
    myCanRecData->cnt=0;
    myCanRecData->datasendflag=LongDataSend_RTS;//标志位接收到RTS，
    if (!myTimer->isActive())
        myTimer->start(10);
}
//=================================================================================
//当接收到RTS后发送给请求数据段的CTS
int QmyCanComm::sendTM_CTS(unsigned int canID,unsigned char sendpacketsnum,unsigned char sendpacketstartnum)
{
    // if (mycom==NULL)  return -3;
    //如果建立了连接或者正在传输数据
    Q_ASSERT(myCanRecData!=NULL);
    uchar buff[8];
    buff[0]=17;
    buff[1]=sendpacketsnum;
    buff[2]=sendpacketstartnum;
    buff[3]=0xFF;
    buff[4]=0xFF;
    buff[5]= myCanRecData->pgn[0];
    buff[6]= myCanRecData->pgn[1];
    buff[7]= myCanRecData->pgn[2];
    int a=sendData(canID,buff,8);
    myCanRecData->datasendflag=LongDataSend_CTS;//标志位接收到RTS
    myCanRecData->sendpacketstartnum=sendpacketstartnum;
    myCanRecData->havesendrecievepacketnum=0;
    myCanRecData->sendpacketsnum=sendpacketsnum;
    myCanRecData->cnt=0;
    return a;
}
//=================================================================================================================
//当接收到数据DT
void QmyCanComm::recieveTM_DT(uchar *buff)
{
    //收数时长数据处理的标识应该为	 正在收数中LongDataSend_DT   等待接收第一包数据
    if(myCanRecData!=NULL&&myCanRecData->data!=NULL)
    {
        if (myCanRecData->datasendflag==LongDataSend_CTS)    //若是发送了CTS怎开始收第一包数据
        {
            if(buff[0]==myCanRecData->sendpacketstartnum)	 //收到的是开始的第一包
            {
                myCanRecData->havesendrecievepacketnum++;
                myCanRecData->datasendflag=LongDataSend_DT;//标识正在接受数据
                myCanRecData->cnt=0;
                if(myCanRecData->havesendrecievepacketnum==myCanRecData->sendpacketsnum)	 //如果此次数据发送完毕
                {
                    if (myCanRecData->sendpacketsnum+myCanRecData->sendpacketstartnum-1==myCanRecData->packetsnum)  //长数据包最后一包发送结束
                    {
                        memcpy(myCanRecData->data+(myCanRecData->sendpacketstartnum-1)*7,buff+1,myCanRecData->datalen-(myCanRecData->sendpacketstartnum-1)*7);
                        myCanRecData->cnt=0;
                        myCanRecData->datasendflag=LongDataSend_End;  //标识数据接收完毕
                        emit CanDataReady(myCanRecData->pgn,myCanRecData->data,myCanRecData->datalen,false);
                        qDebug()<<"last pgn data rev:"<<(myCanRecData->pgn[0]|myCanRecData->pgn[1]<<8|myCanRecData->pgn[2]<<16)<<myCanRecData->datalen;
                        qDebug()<<"2-1 send PGN data end sig..";
                        QThread::msleep(100);
                        sendTM_EndOfMsg(changRecIDToSendID(myCanRecData->addr));
                        //发送完毕释放空间
                        if(myCanRecData!=NULL)
                        {
                            if(myCanRecData->data!=NULL)
                            {
                                delete[] myCanRecData->data;
                                myCanRecData->data=NULL;
                            }
                            delete myCanRecData;
                            myCanRecData=NULL;
                        }
                        //发送结束
                    }
                }else
                    memcpy(myCanRecData->data+(myCanRecData->sendpacketstartnum-1)*7,buff+1,7);
            }
        }else if(myCanRecData->datasendflag==LongDataSend_DT)    //若是已正在接收数据
        {
            if(buff[0]==(myCanRecData->sendpacketstartnum+myCanRecData->havesendrecievepacketnum))   //如果顺序接收
            {
                myCanRecData->havesendrecievepacketnum++;
                if(myCanRecData->havesendrecievepacketnum==myCanRecData->sendpacketsnum)	 //如果此次数据发送完毕
                {
                    if ((myCanRecData->sendpacketsnum+myCanRecData->sendpacketstartnum-1)==myCanRecData->packetsnum) //长数据包最后一包发送结束
                    {
                        memcpy(myCanRecData->data+(myCanRecData->sendpacketstartnum+myCanRecData->havesendrecievepacketnum-2)*7,buff+1,myCanRecData->datalen-(myCanRecData->sendpacketstartnum+myCanRecData->havesendrecievepacketnum-2)*7);
                        myCanRecData->cnt=0;
                        myCanRecData->datasendflag=LongDataSend_End;  //标识数据接收完毕
                        emit CanDataReady(myCanRecData->pgn,myCanRecData->data,myCanRecData->datalen,false);
                        qDebug()<<"2 send PGN data end sig.."<<myCanRecData->pgn[0]<<myCanRecData->pgn[1]<<myCanRecData->pgn[2];
                        //qDebug()<<"pointlong  DATA  over signal";
                        QThread::msleep(100);
                        sendTM_EndOfMsg(changRecIDToSendID(myCanRecData->addr));
                        qDebug()<<"pointlong  DATA  over packet";
                        //发送完毕释放空间
                        if(myCanRecData!=NULL)
                        {
                            if(myCanRecData->data!=NULL)
                            {
                                delete[] myCanRecData->data;
                                myCanRecData->data=NULL;
                            }
                            delete myCanRecData;
                            myCanRecData=NULL;
                        }
                        //发送结束
                    }else if (myCanRecData->sendpacketsnum+myCanRecData->sendpacketstartnum-1<myCanRecData->packetsnum)
                    {
                        memcpy(myCanRecData->data+(myCanRecData->sendpacketstartnum+myCanRecData->havesendrecievepacketnum-2)*7,buff+1,7);
                        myCanRecData->cnt=0;
                        myCanRecData->datasendflag=LongData_DTNextCTS;  //此次CTS数据接收完毕准备发送下次CTS
                        unsigned int a=0;
                        //CTS每次申请	TMCTSSendPackets包 如果是最后一包则发送剩下的
                        if(myCanRecData->packetsnum-(myCanRecData->sendpacketsnum+myCanRecData->sendpacketstartnum-1)>=TMCTSSendPackets)
                            a=TMCTSSendPackets;
                        else if(myCanRecData->packetsnum-(myCanRecData->sendpacketsnum+myCanRecData->sendpacketstartnum-1)>0)
                            a=myCanRecData->packetsnum-(myCanRecData->sendpacketsnum+myCanRecData->sendpacketstartnum-1);
                        sendTM_CTS(changRecIDToSendID(myCanRecData->addr),a,myCanRecData->sendpacketsnum+myCanRecData->sendpacketstartnum);
                        //CTS每次申请	TMCTSSendPackets包 如果是最后一包则发送剩下的
                    }
                }else
                    memcpy(myCanRecData->data+(myCanRecData->sendpacketstartnum+myCanRecData->havesendrecievepacketnum-2)*7,buff+1,7);
            }else
            {
                if(myCanRecData!=NULL)
                {
                    if(myCanRecData->data!=NULL)
                    {
                        delete[] myCanRecData->data;
                        myCanRecData->data=NULL;
                    }
                    delete myCanRecData;
                    myCanRecData=NULL;
                }
            }
        }
    }
}
//================================================================================================================
//当接收到RTS后发送给请求数据段的CTS
int QmyCanComm::sendTM_EndOfMsg(unsigned int canID)
{
    //如果建立了连接或者正在传输数据
    Q_ASSERT(myCanRecData!=NULL);
    uchar buff[8];
    buff[0]=19;
    buff[1]=myCanRecData->datalen;
    buff[2]=(myCanRecData->datalen>>8)&0xff;
    buff[3]=myCanRecData->packetsnum;
    buff[4]=0xFF;
    buff[5]= myCanRecData->pgn[0];
    buff[6]= myCanRecData->pgn[1];
    buff[7]= myCanRecData->pgn[2];
    qDebug()<<"send endo of data1";
    int a=sendData(canID,buff,8);
    qDebug()<<"send endo of data2";
    myCanRecData->datasendflag=LongDataSend_NULL;//发送结束连接断开
    //释放空间
    if(myCanRecData!=NULL)
    {
        qDebug()<<"myCanRecData!=NULL";
        if(myCanRecData->data!=NULL)
        {
            qDebug()<<"myCanRecData->data!=NULL";
            delete[] myCanRecData->data;
            qDebug()<<"delete  myCanRecData->data 1";
            myCanRecData->data=NULL  ;
        }
        delete myCanRecData;
        qDebug()<<"delete  myCanRecData 1";
        myCanRecData=NULL;
    }
    return a;
}

//==================================================================================================================
//切换canid的源地址与目的地址，把接收到的CANID种的目的地址源地址顺序颠倒把接收ID换成发送ID
unsigned int QmyCanComm::changRecIDToSendID(unsigned int canid)
{
    unsigned char buff[4];
    unsigned char tembuff;
    unsigned int  a;
    memcpy(buff,(unsigned char *)&canid,4);
    tembuff=buff[0];
    buff[0]=buff[1];
    buff[1]=tembuff;
    memcpy(&a,buff,4);
    return a;
}

//广播公告数据接收
void QmyCanComm::recieveBroadcastTM_RTS(unsigned int canID,uchar *buff)
{
    // Q_ASSERT(myCanBroadCastRecData==NULL);
    if (myCanBroadCastRecData!=NULL) return ;
    myCanBroadCastRecData=new P2PCanData;
    memset(myCanBroadCastRecData,0,sizeof(P2PCanData));
    myCanBroadCastRecData->datalen=buff[1]|buff[2]<<8;
    myCanBroadCastRecData->packetsnum=buff[3];
    myCanBroadCastRecData->addr=canID;
    myCanBroadCastRecData->sendpacketstartnum=1;
    myCanBroadCastRecData->sendpacketsnum=myCanBroadCastRecData->packetsnum;
    myCanBroadCastRecData->data=new unsigned char[myCanBroadCastRecData->datalen];
    memset(myCanBroadCastRecData->data,0,myCanBroadCastRecData->datalen);
    myCanBroadCastRecData->pgn[0]=buff[5];
    myCanBroadCastRecData->pgn[1]=buff[6];
    myCanBroadCastRecData->pgn[2]=buff[7];
    myCanBroadCastRecData->cnt=0;
    //if()
    myCanBroadCastRecData->datasendflag=LongDataSend_CTS;//标志位接收到广播公告
}
//广播数据接收
//当接收到数据DT
void QmyCanComm::recieveBroadcastTM_DT(unsigned char *buff)
{
    //收数时长数据处理的标识应该为	 正在收数中LongDataSend_DT   等待接收第一包数据
    if(myCanBroadCastRecData!=NULL&&myCanBroadCastRecData->data!=NULL)
    {
        //判断接收数据
        if(buff[0]==(myCanBroadCastRecData->sendpacketstartnum+myCanBroadCastRecData->havesendrecievepacketnum))   //此次数据未发送完成
        {
            if (myCanBroadCastRecData->datasendflag==LongDataSend_CTS)    //若是发送了公告开始收第一包数据
            {
                if(buff[0]==myCanBroadCastRecData->sendpacketstartnum)	 //收到的是开始的第一包
                {
                    memcpy(myCanBroadCastRecData->data+(myCanBroadCastRecData->sendpacketstartnum-1)*7,buff+1,7);
                    myCanBroadCastRecData->havesendrecievepacketnum++;
                    myCanBroadCastRecData->datasendflag=LongDataSend_DT;//标识正在接受数据
                    myCanBroadCastRecData->cnt=0;
                }
            }else if(myCanBroadCastRecData->datasendflag==LongDataSend_DT)    //若是已正在接收数据
            {
                if(buff[0]==(myCanBroadCastRecData->sendpacketstartnum+myCanBroadCastRecData->havesendrecievepacketnum))   //如果顺序接收
                {
                    myCanBroadCastRecData->havesendrecievepacketnum++;
                    if(myCanBroadCastRecData->havesendrecievepacketnum==myCanBroadCastRecData->sendpacketsnum)	 //如果此次数据发送完毕
                    {
                        memcpy(myCanBroadCastRecData->data+(myCanBroadCastRecData->sendpacketstartnum+myCanBroadCastRecData->havesendrecievepacketnum-2)*7
                               ,buff+1,myCanBroadCastRecData->datalen-(myCanBroadCastRecData->sendpacketstartnum+myCanBroadCastRecData->havesendrecievepacketnum-2)*7);
                        myCanBroadCastRecData->cnt=0;
                        myCanBroadCastRecData->datasendflag=LongDataSend_End;  //标识数据接收完毕
                        emit CanDataReady(myCanBroadCastRecData->pgn,myCanBroadCastRecData->data,myCanBroadCastRecData->datalen,true);
                        QThread::msleep(100);
                        //qDebug()<<"Broadcast  DATA  over";
                        myCanBroadCastRecData->datasendflag=LongDataSend_NULL;
                        //发送广播数据接收完毕释放空间	放在槽函数中释放
                        if(myCanBroadCastRecData!=NULL)
                        {
                            if(myCanBroadCastRecData->data!=NULL)
                            {
                                delete[] myCanBroadCastRecData->data;
                                myCanBroadCastRecData->data=NULL  ;
                            }
                            delete myCanBroadCastRecData;
                            myCanBroadCastRecData=NULL;
                        }
                    }else
                        memcpy(myCanBroadCastRecData->data+(myCanBroadCastRecData->sendpacketstartnum+myCanBroadCastRecData->havesendrecievepacketnum-2)*7,buff+1,7);
                }else
                {
                    //如果数据不是顺序接收则
                    if(myCanBroadCastRecData!=NULL)
                    {
                        if(myCanBroadCastRecData->data!=NULL)
                        {
                            delete[] myCanBroadCastRecData->data;
                            myCanBroadCastRecData->data=NULL  ;
                        }
                        delete myCanBroadCastRecData;
                        myCanBroadCastRecData=NULL;
                    }
                }
            }
        }
    }
}
