
#include "qfilesavethead.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>
#include "MsgSignals.h"
#include <unistd.h>
#include <QQueue>
extern QQueue<SerialDataRev> SerialDataQune;
extern QMutex gMutex;
const int CHUNK = 256*1024;
const int WRITEBYTE = 128*1024;
QFileSaveThread::QFileSaveThread(QObject *parent)
    : QThread(parent)
{
    m_bStop = false;

    m_bOpen = false;
    qRegisterMetaType<CanDataBody>("CanDataBody");//自定义类型需要先注册
    recordManager=new RecordManager();
    connect(recordManager,&RecordManager::creatFileSig,this,&QFileSaveThread::onCreatNewFile);
    recordManager->start();
    // 每100ms 将大缓存数据搬入小缓冲

    writeBuffer=new char[CHUNK];
    /* ---------- 新增定时器：定期 flush ---------- */
    m_flushTimer = new QTimer(this);                     // 由 QThread 对象托管
    m_flushTimer->setInterval(1000);               // 10 s 一次
    connect(m_flushTimer, &QTimer::timeout,
            this,          &QFileSaveThread::onFlushTimeout,
            Qt::QueuedConnection);                      // 保证跨线程安全
    m_flushTimer->start();
    /* ------------------------------------------------*/
}

QFileSaveThread::~QFileSaveThread()
{
    stopRecord();
    wait();             // 等 run() 退出
    CloseFile();
    delete[] writeBuffer;
    delete recordManager;
}
/* ----------------- 定时 flush 槽 ----------------- */
void QFileSaveThread::onFlushTimeout()
{
    static int loopcount=0;
    loopcount++;
    if (!m_qsFilePath.isEmpty() && !QFile::exists(m_qsFilePath)) {
        CreatFile(m_qsFilePath);
    }
    if(loopcount%15==0)
    {
        QMutexLocker lock(&m_mutex);
        // 定时检查：若文件被删除，则重建同名文件
        if (m_file.isOpen()) {
            m_file.flush();                     // 把文件缓冲写到磁盘
        }
    }

    static quint64 lastProd = 0, lastCons = 0;

    quint64 p = prodBytes.load();
    quint64 c = consBytes.load();

    int deltaP  = int(p - lastProd);
    int deltaC  = int(c - lastCons);

    lastProd = p;
    lastCons = c;

    qDebug().nospace()
            << "[STAT] prod " << deltaP / 1024 << " KiB/s  "
            << "cons "       << deltaC / 1024 << " KiB/s  ";

}
void QFileSaveThread::startRecord()
{
    m_bStop = false;
    start(QThread::TimeCriticalPriority);
}

void QFileSaveThread::stopRecord()
{
    m_bStop = true;
    /* 唤醒 run() 一次，防止死等 */
    //m_usedSpace.release();
}

void QFileSaveThread::onCreatNewFile(QString fileName)
{
    CloseFile();
    CreatFile(fileName);
}

void QFileSaveThread::onRevCpuinfo(double usedPer)
{
    cpuUsedpercent=usedPer;
    if(cpuUsedpercent>50)
        qDebug()<<"CPU USED Percent :"<<cpuUsedpercent;
}

// 源文件实现
QByteArray QFileSaveThread::packSerial(const SerialDataRev &serialData)
{
    static qint64 recvCnt = 0;
    if (++recvCnt % 1000 == 0)
        qDebug() << "[Serial] total frames:" << recvCnt;

    QByteArray line = recordManager->getRecordData(serialData).toLocal8Bit();
    line.append('\n');                   // 每帧一行
    return line;                         // 直接返回
}

void QFileSaveThread::revCANData(CanDataBody canData)
{
    if(!m_bStop)
    {
        //QString strData=recordManager->getRecordData(canData);
        //saveStringData(strData);
    }
}




void QFileSaveThread::CloseFile()
{
    QMutexLocker fileLock(&m_mutex);
    if (!m_bOpen || !m_file.isOpen()) return;

    m_file.flush();
    m_file.close();
    m_bOpen = false;
}

bool QFileSaveThread::CreatFile(QString qsFilePath)
{
    m_qsFilePath = qsFilePath;
    QFileInfo fi(qsFilePath);
    QDir dir = fi.dir();
    if (!dir.exists()) {
        // 递归创建目录
        if (!QDir().mkpath(dir.absolutePath())) {
            qWarning() << "[FileSave] Failed to create directory"
                       << dir.absolutePath();
            return false;
        }
    }
    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen())
        m_file.close();
    m_file.setFileName(qsFilePath);
    m_bOpen = m_file.open(QIODevice::Append | QIODevice::ReadWrite);
    qDebug() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
    qDebug() << "Creat save file" << qsFilePath;
    qDebug() << "Open file" << m_bOpen ;
    qDebug() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
    return m_bOpen;
}

void QFileSaveThread::run()
{
    const qint64 FLUSH_INTERVAL_MS = 10 * 1000;  // 10秒
    QElapsedTimer timer;
    timer.start();
    qint64 lastFlush = 0;
    //int bytesWritten=0;
    int loopCount=0;
    int bufferUsed = 0;
    while (!m_bStop) {
        // 当前 writeBuffer 已用字节
        gMutex.lock();
        while(!SerialDataQune.empty() && bufferUsed<(CHUNK-1024))
        {
            loopCount++;
            QByteArray line = packSerial(SerialDataQune.dequeue());
            int  len  = line.size();
            if(len>1024)
            {
                qDebug()<<"error,lose Data"<<line.toHex();
                break;
            }
            // 复制到缓存

            memcpy(writeBuffer + bufferUsed, line.constData(), len);
            bufferUsed += len;
            prodBytes += len;
        }
        gMutex.unlock();
        //qDebug()<<"loopCount :"<<loopCount;
        if (m_bStop) break;

        /* ---------- 满足阈值就落盘 ---------- */
        if (bufferUsed >= WRITEBYTE && m_file.isOpen()) {
            QMutexLocker fl(&m_mutex);
            m_file.write(writeBuffer, bufferUsed);
            consBytes += bufferUsed;
            bufferUsed = 0;
        }
        /* 周期 flush（避免频繁磁盘刷新） */
        qint64 now = timer.elapsed();
        if (now - lastFlush >= FLUSH_INTERVAL_MS) {
            if (bufferUsed > 0 && m_file.isOpen()) {
                QMutexLocker fl(&m_mutex);
                m_file.write(writeBuffer, bufferUsed);
                consBytes += bufferUsed;
                bufferUsed = 0;
            }
            lastFlush = now;
        }
        // 如果这一轮没有处理任何数据，则 sleep 一小会降低CPU占用
        if (loopCount<5) {
            QThread::msleep(10); // 10ms 视系统而定，可调为 5~50ms
        }
        else if (loopCount<50) {
            QThread::msleep(2); // 10ms 视系统而定，可调为 5~50ms
        }

        loopCount=0;
    }
    // 退出前一次性写完所有残余
    CloseFile();
}
int QFileSaveThread::diskUsedPercent()
{
    return recordManager->diskUsedPercent;
}
int QFileSaveThread::diskRemains()
{
    return recordManager->diskAll-recordManager->diskUsed;
}

bool QFileSaveThread::sdCardStat()
{
    return recordManager->isSDCardOK;
}
void QFileSaveThread::delAllFiles()
{
    QMutexLocker fl(&m_mutex);
    if (m_file.isOpen())
        m_file.close();

    QString del_file = gPath;
    QDir dir;
    if (dir.exists(del_file))
    {
        dir.setPath(del_file);
        dir.removeRecursively();
    }
}
