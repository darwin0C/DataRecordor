#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include "MsgSignals.h"
#include <QQueue>
#include <QMutex>
#include "versionutil.h" // 引入头文件
QQueue<SerialDataRev> SerialDataQune;
QMutex gMutex;
bool SDCardStatus=true;
int ledBlankTimes = 0;



// 全局常量，编译时由编译器插入当日日期和时间
const QString BUILD_DATE = QStringLiteral(__DATE__);  // 格式如 "Jul 18 2025"
const QString BUILD_TIME = QStringLiteral(__TIME__);  // 格式如 "16:23:45"
const QString gSoftVer="V2.1.0 ";

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
    //Test();
    startcpuMonitor();
    QString versiontime =getBuildDateTime().toString("yyyy-MM-dd HH:mm:ss");
    qDebug()<<"SoftVer:"<<gSoftVer+versiontime;
    // --- 一行代码搞定版本管理 ---
    m_isUpdateSuccess= VersionUtil::checkAndUpdate(gPath,gSoftVer);
}

QDateTime MainWindow::getBuildDateTime()
{
    QString dtStr = QString("%1 %2")
            .arg(BUILD_DATE.simplified())   // 去掉多余空格，变成 "Jul 18 2025"
            .arg(BUILD_TIME);               // "Jul 18 2025 16:23:45"

    // 使用 C 语言环境（英语）解析“MMM d yyyy hh:mm:ss”
    QLocale locale(QLocale::C);
    return  locale.toDateTime(dtStr, "MMM d yyyy hh:mm:ss");
}
void MainWindow::startcpuMonitor()
{
    cpuThread= new QThread(this);
    // 把 monitor 移到后台线程
    monitor = new QCpuMonitor();
    monitor->moveToThread(cpuThread);

    // 线程启动后调用 monitor->start(...)
    connect(cpuThread, &QThread::started, [=]() {
        monitor->start(1000*5);  //
    });

    // 线程结束后自动 delete monitor 与 delete cpuThread
    connect(cpuThread, &QThread::finished, monitor, &QObject::deleteLater);
    connect(cpuThread, &QThread::finished, cpuThread, &QObject::deleteLater);

    // 监控信号连到主线程 UI 更新
    connect(monitor, &QCpuMonitor::cpuUsage, this, [this](double cpuUsed){
        if(cpuUsed>5)
            emit MsgSignals::getInstance()->sendCpuinfo(cpuUsed);
    });

    // 主窗口析构时停止线程
    connect(this, &QObject::destroyed,cpuThread, &QThread::quit);

    // 启动后台线程
    cpuThread->start();
}
void MainWindow::Test()
{
    QTimer *timerTest=new QTimer();
    connect(timerTest,&QTimer::timeout,this,[&](){sendtestData();});
    timerTest->start(1);
}
void MainWindow::sendtestData()
{
    static qint64 index=0;
    uint id=0x0c000102;
    index++;
    QByteArray array(8,0);
    memcpy(array.data(),&index,8);
    //QByteArray array((char *)index,8);
    com->sendRecordCanData(id,(uchar *)array.data(),8);

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
        timerStatus->start(2000); // 每秒触发一次
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
        mySaveDataThread=new QFileSaveThread(this);

        qRegisterMetaType<SerialDataRev>("SerialDataRev");//自定义类型需要先注册
        connect(MsgSignals::getInstance(),&MsgSignals::sendCpuinfo,mySaveDataThread,&QFileSaveThread::onRevCpuinfo);
        connect(this,&MainWindow::delAllFilesSig,mySaveDataThread,&QFileSaveThread::delAllFiles);
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
    if (m_isUpdateSuccess)
    {
        ledBlankTimes++;
        // 在 5 秒内 (100ms * 50 = 5000ms)
        if (ledBlankTimes <= 50)
        {
            // 每一帧(100ms)都调用一次，实现快闪
            blankLED();
        }
        else
        {
            m_isUpdateSuccess = false;
            ledBlankTimes = 20; // 直接跳到正常闪烁的起始点
        }
        return; // 处于更新显示阶段时，不执行下方的正常逻辑
    }

    // --- 以下是原有的正常运行逻辑 ---
    ledBlankTimes++;
    if(ledBlankTimes < 20)
    {
        blankLED();
    }
    else if(ledBlankTimes % 20 == 0) // 每 2 秒 (20 * 100ms) 闪烁一次
    {
        blankLED();

        if(ledBlankTimes == 300)
        {
            ledBlankTimes = 20;
        }
    }
}
void MainWindow::blankLED()
{
    if(mySaveDataThread == nullptr)
        return;

    static bool ledon = false;
    QString ledOnStr = ledGreen_on;
    QString ledOffStr = led_off;
    QString ledBalnkStr;

    // --- 新增：更新成功时的灯光颜色切换 ---
    if (m_isUpdateSuccess)
    {
        if (ledon) {
            ledBalnkStr = ledGreen_on;
            ledon = false;
        } else {
            ledBalnkStr = ledRed_on; // 更新成功：红绿快速交替
            ledon = true;
        }
    }
    // --- 原有逻辑 ---
    else if(mySaveDataThread->sdCardStat())
    {
        if(mySaveDataThread->diskRemains() < diskMinFree)
            ledOnStr = ledRed_on;
        else if(mySaveDataThread->diskUsedPercent() > 70)
            ledOnStr = ledYellow_on;

        if(ledon) {
            ledBalnkStr = ledOnStr;
            ledon = false;
        } else {
            ledBalnkStr = ledOffStr;
            ledon = true;
        }
    }
    else
    {
        ledBalnkStr = ledRed_on; // SD卡异常
    }

#ifdef LINUX_MODE
    system(ledBalnkStr.toLatin1().data());
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
    SDCardStatus=(dataToSend.sdCardStat==0x0F);
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
