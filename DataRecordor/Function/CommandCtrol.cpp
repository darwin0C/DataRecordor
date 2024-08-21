#include "CommandCtrol.h"
#include "MsgSignals.h"
#include "commanager.h"

CommandCtrol::CommandCtrol(QObject *parent) : QObject(parent)
{
    connect(MsgSignals::getInstance(),&MsgSignals::commandDataSig,this,&CommandCtrol::dataHandle);
    connect(&statTimer,&QTimer::timeout,this,&CommandCtrol::timeStatHandle);

}
void CommandCtrol::dataHandle(QByteArray array)
{
    quint32 len=array.size();
    if(len==0)
        return;
    int cmdCode=array[0];
    if(cmdCode==0x07)//系统授时（射击初始条件）
    {
        if (len < sizeof(TimeSetCMD))
            return;
        TimeSetCMD commandData;
        memcpy(&commandData, array.constData(), sizeof(TimeSetCMD));
        sysTimeSetHandle(commandData);
    }
    else if(cmdCode==0xC3)//查询火炮综合信息命令
    {
        if (len < sizeof(CommandDataRequre))
            return;
        CommandDataRequre commandData;
        memcpy(&commandData, array.constData(), sizeof(TimeSetCMD));
    }
}

void CommandCtrol::sysTimeSetHandle(const TimeSetCMD &commandData)
{

}

void CommandCtrol::dataRequreHandle(const CommandDataRequre &commandData)
{
    if(commandData.msgReportCtrl==1)
    {
        setAutoReport(true);
    }
    else
    {
        setAutoReport(false);
    }
    if(commandData.requreMethod==0)//查询当前值
    {
        requreCurrentData(commandData.requreData);
    }
    else if(commandData.requreMethod==1)//查询历史值
    {
        TimeCondition *timeConditionPtr=new TimeCondition(TimeFormatTrans::getDateTime(commandData.startTime), TimeFormatTrans::getDateTime(commandData.startTime));
        requreHistoryData(commandData.requreData,timeConditionPtr);
        delete  timeConditionPtr;
    }
}

void CommandCtrol::requreCurrentData(int dataType)
{
    switch (dataType) {
    case 0://0:属性信息
        break;
    case 1://1:设备工作状态
        break;
    case 2://2:报警信息
        break;
    case 3://3:设备累计工作时间
        break;
    case 4://4:调炮数据
        break;
    case 5://5:射击数据
        break;
    case 0xFF://0xFF:查询全部信息
        break;
    default:
        break;
    }
}

void CommandCtrol::requreHistoryData(int dataType,TimeCondition *timeConditionPtr)
{
    switch (dataType) {
    case 0://0:属性信息
        break;
    case 1://1:设备工作状态
        break;
    case 2://2:报警信息
        break;
    case 3://3:设备累计工作时间
        break;
    case 4://4:调炮数据
        break;
    case 5://5:射击数据
        break;
    case 0xFF://0xFF:查询全部信息
        break;
    default:
        break;
    }
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
    sendData.selfUniqueID=selfUniqueID;
    sendData.dataFlag=1;
    sendData.dataType=0;
    sendData.dataPacketIndedx=0;
    QByteArray array(reinterpret_cast<const char*>(&sendData), sizeof(Send2CommandData));
    int count;
    QByteArray statArray=deviceManager.getErrorDeviceStat(count);
    if(count>0)
    {
        quint8 deviceNum=count;
        array.append(deviceNum);
        array.append(statArray);
    }
    ComManager::instance()->sendData2Command(0xC4,reinterpret_cast<unsigned char*>(array.data()),array.size());
}
