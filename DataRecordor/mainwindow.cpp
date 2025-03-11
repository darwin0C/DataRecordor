#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include "MsgSignals.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    com=ComManager::instance();
    startRecord();
    startLEDThread();
    startStatus();
    startCommandCtrl();

    mp_TCPServer = new QTcpServer();
    if(!mp_TCPServer->listen(QHostAddress::Any, 8080))
    {

    }
    connect(mp_TCPServer, SIGNAL(newConnection()), this, SLOT(ServerNewConnection()));
    connect(MsgSignals::getInstance(),&MsgSignals::sendLEDStatSig,this,&MainWindow::changeLEDStat);
}
void MainWindow::ServerNewConnection()
{
    mp_TCPSocket = mp_TCPServer->nextPendingConnection();
    QString clintName=mp_TCPSocket->peerAddress().toString();
    socket_Send_Data("connected to server\n");
    qDebug()<<(clintName.remove(0,7));
    QObject::connect(mp_TCPSocket, &QTcpSocket::readyRead, this, &MainWindow::socket_Read_Data);
    QObject::connect(mp_TCPSocket, &QTcpSocket::disconnected, this, &MainWindow::socket_Disconnected);
}
void MainWindow::socket_Read_Data()
{
    QByteArray buffer;
    //读取缓冲区数据
    buffer = mp_TCPSocket->readAll();
    if(!buffer.isEmpty())
    {
        QString dataRev=QString(buffer);
        socket_Send_Data("received data from client: "+dataRev);
        qDebug()<<(dataRev);
        if(dataRev.remove(QRegExp("\\s")).toUpper()=="EF0000FFFFFCFFFF")
        {
            emit  delAllFilesSig();
        }
    }
}
void MainWindow::socket_Send_Data(QString dataSend)
{
    QByteArray ba = dataSend.toLatin1();
    char *sendData=ba.data();

    mp_TCPSocket->write(sendData);
    mp_TCPSocket->flush();
}
void MainWindow::socket_Disconnected()
{
    qDebug()<<"socket_Disconnected ";
}

void MainWindow::changeLEDStat()
{
    ledBlankTimes = 0;
}
void MainWindow::startLEDThread()
{
    // 创建线程实例
    ledTimerThread = new QThread(this);

    // 创建 QTimer 实例，关联到新线程
    ledTimer = new QTimer();
    ledTimer->moveToThread(ledTimerThread);

    // 在线程启动后启动 QTimer
    connect(ledTimerThread, &QThread::started, [this]() {
        ledTimer->start(100);  // 设置定时器的时间间隔为 100ms
    });

    // 保证槽函数在主线程中执行，避免子线程操作 GUI 的问题
    connect(ledTimer, &QTimer::timeout, this, &MainWindow::timerUpdate, Qt::QueuedConnection);

    // 启动线程的事件循环
    ledTimerThread->start();
}
void MainWindow::startStatus()
{
    // 创建定时器和线程，并将它们的父对象设置为 this，避免内存泄漏
    timerStatus = new QTimer();
    StatusTimerThread = new QThread(this);

    // 将定时器移动到新线程
    timerStatus->moveToThread(StatusTimerThread);

    // 使用 lambda 表达式在线程启动后启动定时器
    connect(StatusTimerThread, &QThread::started, [this]() {
        timerStatus->start(1000); // 每秒触发一次
    });

    // 连接定时器的超时信号到状态发送槽函数
    connect(timerStatus, &QTimer::timeout, this, &MainWindow::timerSendStatus, Qt::QueuedConnection);

    // 启动线程
    StatusTimerThread->start();
}
void MainWindow::startCommandCtrl()
{
    commandCtrol=new CommandCtrol();
    commandThread= new QThread(this);
    // 将定时器移动到新线程
    commandCtrol->moveToThread(commandThread);
    commandThread->start();
}


MainWindow::~MainWindow()
{
    StatusTimerThread->terminate();
    StatusTimerThread->deleteLater();
    ledTimerThread->terminate();
    ledTimerThread->deleteLater();
    commandThread->terminate();
    commandThread->deleteLater();
    delete ui;
}


void MainWindow::startRecord()
{
    if(mySaveDataThread==nullptr)
    {
        mySaveDataThread=new QFileSaveThead(this);

        qRegisterMetaType<SerialDataRev>("SerialDataRev");//自定义类型需要先注册
        connect(MsgSignals::getInstance(),&MsgSignals::serialDataSig,mySaveDataThread,&QFileSaveThead::revSerialData);
        connect(this,&MainWindow::delAllFilesSig,mySaveDataThread,&QFileSaveThead::delAllFiles);
    }
    mySaveDataThread->startRecord();
    emit MsgSignals::getInstance()->sendCheckDiskSig();
    // 设置线程的优先级为最高
    mySaveDataThread->setPriority(QThread::HighestPriority);
}


void MainWindow::on_pushButton_2_clicked()
{
    if(mySaveDataThread!=NULL)
        mySaveDataThread->stopRecord();
}

void MainWindow::timerUpdate(void)
{
    ledBlankTimes++;
    if(ledBlankTimes<20)
    {
        blankLED();
    }
    else if(ledBlankTimes%20==0)
    {
        blankLED();

        if(ledBlankTimes==300)
        {
            ledBlankTimes=20;
        }
    }
}
void MainWindow::blankLED()
{
    if(mySaveDataThread==nullptr)
        return;
    static bool ledon=false;
    QString ledOnStr=ledGreen_on;
    QString ledOffStr=led_off;
    QString ledBalnkStr;
    if(mySaveDataThread->sdCardStat())
    {
        if(mySaveDataThread->diskRemains()<diskMinFree)
        {
            //qDebug()<<"LED red ========================"<<ledBalnkStr;
            ledOnStr=ledRed_on;
        }
        else if(mySaveDataThread->diskUsedPercent()>70)
        {
            //qDebug()<<"LED yellow ========================"<<ledBalnkStr;
            ledOnStr=ledYellow_on;
        }
        if(ledon)
        {
            ledBalnkStr=ledOnStr;
            ledon=false;
        }
        else
        {
            ledBalnkStr=ledOffStr;
            ledon=true;
        }
    }
    else
    {
        ledBalnkStr=ledRed_on;
    }
    //qDebug()<<"LED stat ========================"<<ledBalnkStr;
#ifdef LINUX_MODE
    QByteArray cmdby_heartbeat = ledBalnkStr.toLatin1();
    char* charCmd_heartbeat = cmdby_heartbeat.data();
    system(charCmd_heartbeat);
    //qDebug()<<"blankLED"<<charCmd_heartbeat;
#endif
}
unsigned char calculateCheckCode(SerialDataSend* data)
{
    unsigned char sum = 0;
    unsigned char* ptr = (unsigned char*)data;
    for (size_t i = 1; i < sizeof(SerialDataSend) - 1; ++i) {
        sum += ptr[i];
    }
    return sum & 0xFF; // 取低8位
}
void MainWindow::timerSendStatus()
{
    emit MsgSignals::getInstance()->sendCheckDiskSig();
    // 字节和校验函数（计算校验码）
    SerialDataSend dataToSend;
    memset(&dataToSend,0,sizeof(SerialDataSend));
    dataToSend.head=0XD1;
    dataToSend.equStat=mySaveDataThread->sdCardStat()?0x0F:0xFF;
    dataToSend.sdCardStat=mySaveDataThread->sdCardStat()?0x0F:0xFF;
    dataToSend.sdCardCapcity=mySaveDataThread->diskRemains();
    dataToSend.usedPercentage=mySaveDataThread->diskUsedPercent();
    dataToSend.checkCode=calculateCheckCode(&dataToSend);
    QByteArray data((char *)&dataToSend,sizeof(SerialDataSend));

    sendData(data);
}

void MainWindow::sendData(QByteArray dataArray)
{
    //qDebug()<<"sendData to CAN ===========";
    static int comindex=0;
    com->senSerialDataByCom(dataArray,comindex++);
    if(comindex>=3)
        comindex=0;
}
