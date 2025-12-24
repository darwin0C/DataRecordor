#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include "MsgSignals.h"
#include <QQueue>
#include <QFile>
#include <QMutex>
#ifdef LINUX_MODE
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>  // 增加此行，确保文件权限宏可用
#include <sys/types.h> // 增加此行
#endif
#include "versionutil.h" // 引入头文件
QQueue<SerialDataRev> SerialDataQune;
QMutex gMutex;
bool SDCardStatus=true;
int ledBlankTimes = 0;

static const char* GPIO1_PATH = "/sys/class/gpio/gpio1/value";
static const char* GPIO2_PATH = "/sys/class/gpio/gpio2/value";

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
    if(m_isUpdateSuccess) {
        ledBlankTimes = 0;
        m_updateTimer.restart(); // 确保计时从 0 开始
    }
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
    // 停止所有计时器
    if (ledTimer) ledTimer->stop();
    if (timerStatus) timerStatus->stop();

    // 优雅退出线程
    auto stopThread = [](QThread* t) {
        if (t && t->isRunning()) {
            t->quit();
            if (!t->wait(500)) { // 等待 500ms
                t->terminate(); // 实在退不出才强制
            }
        }
    };

    stopThread(StatusTimerThread);
    stopThread(ledTimerThread);
    stopThread(commandThread);
    stopThread(cpuThread);

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
void MainWindow::fastWriteGpio(int gpioNum, bool value) {
#ifdef LINUX_MODE
    const char* path = (gpioNum == 1) ? GPIO1_PATH : GPIO2_PATH;
    int fd = ::open(path, O_WRONLY);
    if (fd >= 0) {
        ::write(fd, value ? "1" : "0", 1);
        ::close(fd);
    }
#endif
}
void MainWindow::timerUpdate(void)
{
    //    if (m_isUpdateSuccess)
    //    {
    //        if (!m_updateTimer.isValid()) {
    //            m_updateTimer.start(); // 第一次进入时启动计时
    //        }
    //        if (m_updateTimer.elapsed() < 5000)
    //        {
    //            blankLED();
    //            return; // 跳过后续正常逻辑
    //        }
    //        else
    //        {
    //            m_isUpdateSuccess = false;
    //            m_updateTimer.invalidate(); // 失效计时器以备下次使用
    //            ledBlankTimes = 20;         // 重置正常闪烁的计数器
    //        }
    //    }

    if (m_isUpdateSuccess) {
        // 第一次进入时启动计时
        if (!m_updateTimer.isValid()) {
            m_updateTimer.start();
        }

        // 5秒判断
        if (m_updateTimer.elapsed() < 1000*10) {
            blankLED(); // 执行红-灭-绿-灭逻辑
        } else {
            m_isUpdateSuccess = false;
            m_updateTimer.invalidate();
            ledBlankTimes = 20; // 回归正常逻辑起始计数
            fastWriteGpio(1, 0); fastWriteGpio(2, 0); // 状态切换瞬间灭灯避免视觉混乱
        }
        return;
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
//void MainWindow::blankLED()
//{
//    if(mySaveDataThread == nullptr) return;

//    static int updateStep = 0; // 用于更新成功时的四步循环
//    static bool normalLedon = false; // 用于正常运行时的亮灭切换

//    QString ledBalnkStr;

//    // ==========================================
//    // 1. 更新成功模式：红 -> 灭 -> 绿 -> 灭
//    // ==========================================
//    if (m_isUpdateSuccess)
//    {
//        switch (updateStep % 4) {
//        case 0: ledBalnkStr = ledRed_on;   break; // 红灯
//        case 1: ledBalnkStr = led_off;    break; // 不亮
//        case 2: ledBalnkStr = ledGreen_on; break; // 绿灯
//        case 3: ledBalnkStr = led_off;    break; // 不亮
//        }
//        updateStep++;
//    }
//    // ==========================================
//    // 2. 正常运行模式
//    // ==========================================
//    else
//    {
//        updateStep = 0; // 重置更新步数
//        QString ledOnStr = ledGreen_on;

//        if(mySaveDataThread->sdCardStat())
//        {
//            if(mySaveDataThread->diskRemains() < diskMinFree)
//                ledOnStr = ledRed_on;
//            else if(mySaveDataThread->diskUsedPercent() > 70)
//                ledOnStr = ledYellow_on;

//            if(normalLedon) {
//                ledBalnkStr = ledOnStr;
//                normalLedon = false;
//            } else {
//                ledBalnkStr = led_off;
//                normalLedon = true;
//            }
//        }
//        else
//        {
//            ledBalnkStr = ledRed_on;
//        }
//    }

//    // 执行系统命令
//#ifdef LINUX_MODE
//    system(ledBalnkStr.toLatin1().data());
//#endif
//}
void MainWindow::blankLED()
{
    if(mySaveDataThread == nullptr) return;

    static int updateStep = 0; // 用于四步循环计数
    static bool normalLedon = false;

    // ==========================================
    // 1. 更新成功模式：红 -> 灭 -> 绿 -> 灭 (100ms/步)
    // ==========================================
    if (m_isUpdateSuccess) {
        switch (updateStep % 4) {
        case 0: fastWriteGpio(1, 0); fastWriteGpio(2, 1); break; // 红 (GPIO2为红)
        case 1: fastWriteGpio(1, 0); fastWriteGpio(2, 0); break; // 灭
        case 2: fastWriteGpio(1, 1); fastWriteGpio(2, 0); break; // 绿 (GPIO1为绿)
        case 3: fastWriteGpio(1, 0); fastWriteGpio(2, 0); break; // 灭
        }
        updateStep++;
        return;
    }

    // ==========================================
    // 2. 正常运行模式
    // ==========================================
    updateStep = 0; // 重置更新步数
    bool targetG1 = false; // 绿
    bool targetG2 = false; // 红

    if(mySaveDataThread->sdCardStat()) {
        normalLedon = !normalLedon; // 状态翻转
        if(normalLedon) {
            if(mySaveDataThread->diskRemains() < diskMinFree) {
                targetG2 = true; // 空间不足：红闪
            } else if(mySaveDataThread->diskUsedPercent() > 70) {
                targetG1 = true; targetG2 = true; // 空间警告：黄闪
            } else {
                targetG1 = true; // 正常：绿闪
            }
        }
    } else {
        targetG2 = true; // SD卡异常：红常亮
    }

    fastWriteGpio(1, targetG1);
    fastWriteGpio(2, targetG2);
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
