#include "CommandCtrol.h"
#include "MsgSignals.h"
#include "commanager.h"
#include "inisettings.h"

CommandCtrol::CommandCtrol(QObject *parent) : QObject(parent)
{
    connect(MsgSignals::getInstance(),&MsgSignals::commandDataSig,this,&CommandCtrol::dataHandle);
    connect(MsgSignals::getInstance(),&MsgSignals::startAutoSend,this,&CommandCtrol::setAutoReport);

    connect(&statTimer,&QTimer::timeout,this,&CommandCtrol::timeStatHandle);

    connect(&eventInfo,&EventInfo::sendCommandDataSig,this,&CommandCtrol::autoSendCommandDataHandle);

    initData();
    setAutoReport(false);
    quint16 SelfAddrCode =iniSettings::Instance()->getSelfAttribute();
    commandCode=0xF0 & SelfAddrCode;
}

void CommandCtrol::initData()
{
    selfAttribute= iniSettings::Instance()->getSelfAttribute();
    selfUniqueID= iniSettings::Instance()->getSelfUniqueID();
}

void CommandCtrol::dataHandle(quint16 sendCode,quint16 cmdCode,QByteArray array)
{
    commandCode=sendCode;
    quint32 len=array.size();
    if(len==0)
        return;
    switch (cmdCode&0xFF) {
    case CMD_Code_SysTimeSet://系统授时（射击初始条件）
        if (len >= sizeof(TimeSetCMD))
        {
            TimeSetCMD commandData;
            memcpy(&commandData, array.constData(), sizeof(TimeSetCMD));
            sysTimeSetHandle(commandData);
        }
        break;
    case CMD_Code_Request://查询火炮综合信息命令
        if (len >= sizeof(CommandDataRequre))
        {
            CommandDataRequre commandData;
            memcpy(&commandData, array.constData(), sizeof(CommandDataRequre));
            dataRequreHandle(commandData);
        }
        break;
    case CMD_Code_SetAttribute://设置属性指令
        if (len >= sizeof(SelfAttributeData))
        {
            SelfAttributeData commandData;
            memcpy(&commandData, array.constData(), sizeof(SelfAttributeData));
            setAttributeHandle(commandData);
        }
        break;
    }

}

//系统授时（射击初始条件）
void CommandCtrol::sysTimeSetHandle(const TimeSetCMD &commandData)
{
    Q_UNUSED(commandData);

}

//处理指令查询命令
void CommandCtrol::dataRequreHandle(const CommandDataRequre &commandData)
{
    if(commandData.msgReportCtrl==1)
    {
        emit MsgSignals::getInstance()->startAutoSend(true);
        //setAutoReport(true);
        eventInfo.setAutoReport(true);
    }
    else
    {
        emit MsgSignals::getInstance()->startAutoSend(false);
        //setAutoReport(false);
        eventInfo.setAutoReport(false);
    }

    if(commandData.requreMethod==DataType_RTData)//查询当前值
    {
        SendRTDataToCommand(commandData.requreData,commandData.deviceAddress);
    }
    else if(commandData.requreMethod==DataType_HistoryData)//查询历史值
    {
        TimeCondition *timeConditionPtr=new TimeCondition(TimeFormatTrans::getDateTime(commandData.startTime), TimeFormatTrans::getDateTime(commandData.startTime));
        sendHistoryDataToCommand(commandData.requreData,commandData.deviceAddress,timeConditionPtr);
        delete  timeConditionPtr;
    }
}

void CommandCtrol::setAttributeHandle(const SelfAttributeData &commandData)
{
    selfAttribute=commandData.attribute;
    selfUniqueID= QString::fromUtf8(commandData.uniqueID);
    iniSettings::Instance()->setAttribute(selfAttribute,selfUniqueID);
}

void CommandCtrol::sendHistoryDataToCommand(int dataFlag,int deviceAddress,TimeCondition *timeConditionPtr)
{
    Send2CommandData sendData;
    sendData.selfAttribute=iniSettings::Instance()->getSelfAttribute();
    //sendData.selfUniqueID=selfUniqueID;
    stringToCharArray(iniSettings::Instance()->getSelfUniqueID(),sendData.selfUniqueID,10);
    sendData.dataType=DataType_HistoryData;
    if(dataFlag==DataFlag_AllData)//查询所有信息
    {
        for(int i=0;i<5;i++)
        {
            sendHisData(sendData,i,deviceAddress,timeConditionPtr);
        }
    }
    else
    {
        sendHisData(sendData,dataFlag,deviceAddress,timeConditionPtr);
    }
}

void CommandCtrol::sendHisData(Send2CommandData sendData,int dataFlag,int deviceAddress,TimeCondition *timeConditionPtr)
{
    if(dataFlag==DataFlag_Attribute || dataFlag==DataFlag_TotalWorkTime)//属性信息、设备累计工作时间与实时数据相同
        sendCommandData(sendData,requreCurrentData(dataFlag,deviceAddress));
    else if(dataFlag==DataFlag_GunMoveData)
    {
        QList<GunMoveData> gunMoveDataList= eventInfo.getHistoryGunMoveData(timeConditionPtr);
        for(const GunMoveData &gunMoveData:gunMoveDataList)
        {
            QByteArray array(reinterpret_cast<const char*>(&gunMoveData), sizeof(GunMoveData));
            sendCommandData(sendData,array);
        }
    }
    else if(dataFlag==DataFlag_GunshootData)
    {
        QList<GunFiringData> gunGunFiringDataList= eventInfo.getHistoryGunFiringData(timeConditionPtr);
        for(const GunFiringData &gunFiringData:gunGunFiringDataList)
        {
            QByteArray array(reinterpret_cast<const char*>(&gunFiringData), sizeof(GunFiringData));
            sendCommandData(sendData,array);
        }
    }
}

void CommandCtrol::SendRTDataToCommand(int dataFlag,int deviceAddress)
{
    Send2CommandData sendData;
    sendData.selfAttribute=selfAttribute;
    //sendData.selfUniqueID=selfUniqueID;
    stringToCharArray(selfUniqueID,sendData.selfUniqueID,10);
    sendData.dataType=DataType_RTData;

    if(dataFlag==DataFlag_AllData)//查询所有信息
    {
        for(int i=0;i<5;i++)
        {
            sendData.dataFlag=i;
            sendData.dataPacketIndedx=i+1;
            sendCommandData(sendData,requreCurrentData(i,deviceAddress));
        }
    }
    else
    {
        sendData.dataFlag=dataFlag;
        sendData.dataPacketIndedx=0;
        sendCommandData(sendData,requreCurrentData(dataFlag,deviceAddress));
    }
}
void CommandCtrol::autoSendCommandDataHandle(int dataFlag, QByteArray dataArray)
{
    Send2CommandData sendData;
    sendData.selfAttribute=selfAttribute;
    //sendData.selfUniqueID=selfUniqueID;
    stringToCharArray(selfUniqueID,sendData.selfUniqueID,10);
    sendData.dataFlag=dataFlag;
    sendData.dataType=DataType_RTData;
    sendData.dataPacketIndedx=0;
    QByteArray array(reinterpret_cast<const char*>(&sendData), sizeof(Send2CommandData));
    array.append(dataArray);
    qDebug()<<"autoSendCommandDataHandle";
    ComManager::instance()->sendData2Command(commandCode,CMD_Code_Report,reinterpret_cast<unsigned char*>(array.data()),array.size());

}


void CommandCtrol::sendCommandData(Send2CommandData sendData, QByteArray dataArray)
{
    QByteArray array(reinterpret_cast<const char*>(&sendData), sizeof(Send2CommandData));
    array.append(dataArray);
    qDebug()<<"sendCommandData";
    ComManager::instance()->sendData2Command(commandCode,CMD_Code_Report,reinterpret_cast<unsigned char*>(array.data()),array.size());
}

QByteArray CommandCtrol::requreCurrentData(int dataFlag,int deviceAddress)
{
    qDebug()<<"requreCurrentData"<<dataFlag<<deviceAddress;
    QByteArray dataArray;
    switch (dataFlag) {
    case DataFlag_Attribute://0:属性信息
    {
        SelfAttributeData selfAttributeData;
        selfAttributeData.attribute=selfAttribute;
        //selfAttributeData.uniqueID=selfUniqueID;
        stringToCharArray(selfUniqueID,selfAttributeData.uniqueID,10);
        dataArray.append(QByteArray(reinterpret_cast<const char*>(&selfAttributeData), sizeof(SelfAttributeData)));
    }
        break;
    case DataFlag_WorkStat://1:设备工作状态
    {
        int count=0;
        QByteArray statArray=deviceManager.getDeviceStat(deviceAddress,count);
        qint8 countByte=count & 0xFF;
        dataArray.append(countByte);
        dataArray.append(statArray);
    }
        break;
    case DataFlag_AlarmInfo://2:报警信息
    {
        dataArray=eventInfo.getCurrentAlarmData();
    }
        break;
    case DataFlag_TotalWorkTime://3:设备累计工作时间
    {
        int count=0;
        QByteArray statArray=deviceManager.getDeviceTotalWorktime(deviceAddress,count);
        qint8 countByte=count & 0xFF;
        dataArray.append(countByte);
        dataArray.append(statArray);
    }
        break;
    case DataFlag_GunMoveData://4:调炮数据
    {
        GunMoveData gunMoveData= eventInfo.getGunMoveData();
        QByteArray statArray=QByteArray(reinterpret_cast<const char*>(&gunMoveData), sizeof(GunMoveData));
        dataArray.append(statArray);
    }
        break;
    case DataFlag_GunshootData://5:射击数据
    {
        GunFiringData gunFiringData= eventInfo.getGunFiringData();
        QByteArray statArray=QByteArray(reinterpret_cast<const char*>(&gunFiringData), sizeof(GunFiringData));
        dataArray.append(statArray);
    }
        break;
    default:
        break;
    }
    return dataArray;
}


void CommandCtrol::setAutoReport(bool enable)
{
    if(enable)
        statTimer.start(2000);
    else
        statTimer.stop();
}

void CommandCtrol::timeStatHandle()
{
    Send2CommandData sendData;
    sendData.selfAttribute=selfAttribute;
    //sendData.selfUniqueID=selfUniqueID;
    stringToCharArray(selfUniqueID,sendData.selfUniqueID,10);
    sendData.dataFlag=DataFlag_WorkStat;
    sendData.dataType=DataType_RTData;
    sendData.dataPacketIndedx=0;
    QByteArray array(reinterpret_cast<const char*>(&sendData), sizeof(Send2CommandData));
    int count;
    QByteArray statArray=deviceManager.getErrorDeviceStat(count);
    if(count>0)
    {
        quint8 deviceNum=count;
        array.append(deviceNum);
        array.append(statArray);
        qDebug()<<"timeStatHandle";
        ComManager::instance()->sendData2Command(commandCode,CMD_Code_Report,reinterpret_cast<unsigned char*>(array.data()),array.size());
    }
}

bool CommandCtrol::stringToCharArray(const QString& source, char* dest, int maxSize) {
    // 检查参数有效性
    if (!dest || maxSize <= 0) {
        return false;
    }

    // 将QString转换为UTF-8编码的字节数组
    QByteArray byteArray = source.toUtf8();

    // 检查源字符串长度是否超过目标数组大小
    if (byteArray.size() >= maxSize) {
        // 截断到maxSize-1，留空间给结束符
        strncpy(dest, byteArray.constData(), maxSize - 1);
        dest[maxSize - 1] = '\0'; // 确保字符串以null终止
    } else {
        // 完整复制
        strcpy(dest, byteArray.constData());
    }

    return true;
}
