
#include "qfilesavethead.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>
#include "MsgSignals.h"

const int CHUNK = 256*1024;
QFileSaveThead::QFileSaveThead(QObject *parent)
    : QThread(parent)
    , m_usedSpace(0)
{
    m_bStop = false;
    m_nCacheSize = 1 * 1024 * 1024; //1MB
    m_bOpen = false;

    connect(&timer,&QTimer::timeout,this,[this](){revSerialData();});
    timer.start(5000);

    qRegisterMetaType<CanDataBody>("CanDataBody");//自定义类型需要先注册
    connect(&recordManager,&RecordManager::creatFileSig,this,&QFileSaveThead::onCreatNewFile);

    connect(&recordManager,&RecordManager::haveDataRev,this,&QFileSaveThead::revSerialData);
    connect(MsgSignals::getInstance(),&MsgSignals::serialDataSig,&recordManager,&RecordManager::revSerialData);

    // 每100ms 将大缓存数据搬入小缓冲
    m_ring=new QTCQueue(m_nCacheSize);
    writeBuffer=new char[CHUNK];
}

QFileSaveThead::~QFileSaveThead()
{
    stopRecord();       // 停止定时器并发信号
    wait();             // 等待 run() 自然退出
    CloseFile();
    delete[] writeBuffer;
    delete m_ring;
}

void QFileSaveThead::startRecord()
{
    m_bStop = false;
    start(QThread::TimeCriticalPriority);
    qDebug()<<"save file thread "<<isRunning();
    //cacheTmr->start(100);    // 每 100ms 触发一次 onCacheTimer()
}

void QFileSaveThead::stopRecord()
{
    m_bStop = true;
    m_usedSpace.release();
}

void QFileSaveThead::onCreatNewFile(QString fileName)
{
    CloseFile();
    CreatFile(fileName);
}

void QFileSaveThead::onRevCpuinfo(double usedPer)
{
    cpuUsedpercent=usedPer;
    if(cpuUsedpercent>50)
        qDebug()<<"CPU USED Percent :"<<cpuUsedpercent;
}

void QFileSaveThead::revSerialData()
{
    QMutexLocker locker(&recordManager.m_mutexDataArray);
    while (!recordManager.dataArrayBuffer.isEmpty() && m_ring->FreeCount() > 0) {
        // 按块移动
        int chunk = qMin<int>(recordManager.dataArrayBuffer.size(),
                              m_ring->FreeCount());
        m_ring->Add(recordManager.dataArrayBuffer.data(), chunk);
        recordManager.dataArrayBuffer.remove(0, chunk);
        m_usedSpace.release();
    }

}

int QFileSaveThead::GetCacheSize() const
{
    return m_nCacheSize;
}

void QFileSaveThead::SetCacheSize(int nCacheSize)
{
    QMutexLocker locker(&m_mutex);
    m_nCacheSize = nCacheSize;
}

void QFileSaveThead::CloseFile()
{
    QMutexLocker fileLock(&m_mutex);
    if (!m_bOpen || !m_file.isOpen()) return;

    char tmp[CHUNK];
    while (m_ring->InUseCount()>0) {
        int got = m_ring->Get(tmp, CHUNK);
        if (got>0) m_file.write(tmp, got);
    }
    // 把 overflow 全部写出
    {
        QMutexLocker dataLock(&recordManager.m_mutexDataArray);
        if (!recordManager.dataArrayBuffer.isEmpty()) {
            m_file.write(recordManager.dataArrayBuffer.constData(),
                         recordManager.dataArrayBuffer.size());
            recordManager.dataArrayBuffer.clear();
        }
    }

    m_file.flush();
    m_file.close();
    m_bOpen = false;
}

bool QFileSaveThead::CreatFile(QString qsFilePath)
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
    qDebug() << "Cache size is" << m_nCacheSize << "byte";
    qDebug() << "Open file" << m_bOpen ;
    qDebug() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
    return m_bOpen;
}

void QFileSaveThead::run()
{
    const qint64 FLUSH_INTERVAL_MS = 10 * 1000;  // 10秒
    QElapsedTimer timer;
    timer.start();
    qint64 lastFlush = 0;

    int loopCounter = 0;
    while (!m_bStop) {
        m_usedSpace.acquire();          // 等待数据
        if (m_bStop) break;

        // 1) 批量从 ring 读，直到读满 CHUNK 或 ring 空
        int got = 0, total = 0;
        int readCounter=0;
        do {
            got = m_ring->Get(writeBuffer + total, CHUNK - total);
            total += got;
            readCounter++;
        } while (/*cpuUsedpercent<70 &&*/ readCounter < 2000 && total < CHUNK);
        if (total > 0 && m_file.isOpen()) {
            QMutexLocker lock(&m_mutex);
            m_file.write(writeBuffer, total);
            qDebug()<<" m_file.write bytes===="<<total;
        }
        // c) 环形空了，搬 overflow
        if (m_ring->InUseCount()==0) {
            QMutexLocker dataLock(&recordManager.m_mutexDataArray);
            if (!recordManager.dataArrayBuffer.isEmpty()) {
                int toMove = qMin(recordManager.dataArrayBuffer.size(),
                                  m_ring->FreeCount());
                m_ring->Add(recordManager.dataArrayBuffer.data(), toMove);
                m_usedSpace.release();
                recordManager.dataArrayBuffer.remove(0, toMove);
            }
        }
        // 时间到达后再做 flush
        loopCounter=0;
        qint64 now = timer.elapsed();
        if (now - lastFlush >= FLUSH_INTERVAL_MS) {
            QMutexLocker lock(&m_mutex);
            if (m_file.isOpen())

                m_file.flush();
            lastFlush = now;
        }

    }
    // 退出前一次性写完所有残余
    CloseFile();
}

int QFileSaveThead::diskUsedPercent()
{
    return recordManager.diskUsedPercent;
}
int QFileSaveThead::diskRemains()
{
    return recordManager.diskAll-recordManager.diskUsed;
}

bool QFileSaveThead::sdCardStat()
{
    return recordManager.isSDCardOK;
}
void QFileSaveThead::delAllFiles()
{
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
