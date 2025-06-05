
#include "qfilesavethead.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QTimer>
#include "MsgSignals.h"
#include <unistd.h>
#include <QQueue>
extern QQueue<SerialDataRev> SerialDataQune;
extern QMutex gMutex;
const int CHUNK = 128*1024;
QFileSaveThread::QFileSaveThread(QObject *parent)
    : QThread(parent)
    , m_usedSpace(0)
{
    m_bStop = false;
    m_nCacheSize = 1 * 1024 * 1024; //1MB

    m_bOpen = false;
    qRegisterMetaType<CanDataBody>("CanDataBody");//自定义类型需要先注册
    recordManager=new RecordManager();
    connect(recordManager,&RecordManager::creatFileSig,this,&QFileSaveThread::onCreatNewFile);
    recordManager->start();
    // 每100ms 将大缓存数据搬入小缓冲
    m_ring=new QTCQueue(m_nCacheSize);
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
    delete   m_ring;
}
/* ----------------- 定时 flush 槽 ----------------- */
void QFileSaveThread::onFlushTimeout()
{
    static int loopcount=0;
    if(loopcount%10==0)
    {
        QMutexLocker lock(&m_mutex);
        if (m_file.isOpen()) {
            m_file.flush();                     // 把文件缓冲写到磁盘
            // 可选：这里也可以打印日志
            // qDebug() << "[FileSave] flush by timer";
#ifdef LINUX_MODE
            ::fdatasync(m_file.handle());       // 真正提交到介质
#endif
        }
    }
    static quint64 lastProd = 0, lastCons = 0;

    quint64 p = prodBytes.load();
    quint64 c = consBytes.load();

    int ringNow = m_ring->InUseCount();
    int deltaP  = int(p - lastProd);
    int deltaC  = int(c - lastCons);

    lastProd = p;
    lastCons = c;

    qDebug().nospace()
            << "[STAT] prod " << deltaP / 1024 << " KiB/s  "
            << "cons "       << deltaC / 1024 << " KiB/s  "
            << "ring "       << ringNow / 1024 << " / " << m_nCacheSize / 1024
            << " KiB  (used " << (ringNow * 100 / m_nCacheSize) << "%)";
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
/* ======================== 公用 PUSH 接口 ======================== */
void QFileSaveThread::pushToRing(char *src, int len)
{
    int offset = 0;
    while (offset < len && !m_bStop) {
        int freeBytes = m_ring->FreeCount();
        if (freeBytes == 0) {
            /* ring 满 => 让出 CPU，等 run() 消费 */
            QThread::yieldCurrentThread();
            continue;
        }
        int chunk = qMin(len - offset, freeBytes);
        m_ring->Add(src + offset, chunk);
        prodBytes += chunk; // ← 新增
        offset += chunk;
        /* 每写入 chunk 字节，给 run() 放行 chunk 次 */
        //m_usedSpace.release(chunk);
    }
}

void QFileSaveThread::onRevCpuinfo(double usedPer)
{
    cpuUsedpercent=usedPer;
    if(cpuUsedpercent>50)
        qDebug()<<"CPU USED Percent :"<<cpuUsedpercent;
}

void QFileSaveThread::revSerialData(SerialDataRev serialData)
{
    if (m_bStop) return;

    static qint64 recvCnt = 0;
    if (++recvCnt % 1000 == 0)
        qDebug() << "[Serial] total frames:" << recvCnt;

    QByteArray data = recordManager->getRecordData(serialData).toLocal8Bit();
    data.append('\n');
    pushToRing(data.data(), data.size());

}

void QFileSaveThread::revCANData(CanDataBody canData)
{
    if(!m_bStop)
    {
        //QString strData=recordManager->getRecordData(canData);
        //saveStringData(strData);
    }
}

int QFileSaveThread::GetCacheSize() const
{
    return m_nCacheSize;
}

void QFileSaveThread::SetCacheSize(int nCacheSize)
{
    QMutexLocker locker(&m_mutex);
    m_nCacheSize = nCacheSize;
}

void QFileSaveThread::CloseFile()
{
    QMutexLocker fileLock(&m_mutex);
    if (!m_bOpen || !m_file.isOpen()) return;

    char tmp[CHUNK];
    while (m_ring->InUseCount() > 0) {
        int got = m_ring->Get(tmp, CHUNK);
        if (got > 0)
            m_file.write(tmp, got);
    }
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
    qDebug() << "Cache size is" << m_nCacheSize << "byte";
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
    int bytesWritten=0;
    int loopCount=0;
    while (!m_bStop) {

        gMutex.lock();
        while(!SerialDataQune.empty())
        {
            if(loopCount++>100)
                break;
            revSerialData(SerialDataQune.dequeue());
        }
        gMutex.unlock();
        qDebug()<<"loopCount :"<<loopCount;
        if (m_bStop) break;
        // 1) 批量从 ring 读，直到读满 CHUNK 或 ring 空
        int got = 0, total = 0;
        /* 把 ring 中可读数据尽量一次搬满 256 KiB */
        do {
            got = m_ring->Get(writeBuffer + total, CHUNK - total);
            total += got;
        } while (got > 0 && total < CHUNK);

        if (total > 0 && m_file.isOpen()) {
            QMutexLocker lock(&m_mutex);
            m_file.write(writeBuffer, total);
            qDebug()<<"bytesWritten :"<<total;
            consBytes += total;// ← 新增
            bytesWritten += total;
            if (bytesWritten >=  4*1024 * 1024) {  // 每 4 MB 强刷一次
#ifdef LINUX_MODE
                ::fdatasync(m_file.handle());       // 真正提交到介质
#endif
                bytesWritten = 0;
            }
        }
        /* 周期 flush（避免频繁磁盘刷新） */
        qint64 now = timer.elapsed();
        if (now - lastFlush >= FLUSH_INTERVAL_MS) {
            CheckFileExists();
            lastFlush = now;
        }
        // 如果这一轮没有处理任何数据，则 sleep 一小会降低CPU占用
        if (loopCount<2) {
            QThread::msleep(10); // 10ms 视系统而定，可调为 5~50ms
        }
        else if (loopCount<10) {
            QThread::msleep(5); // 10ms 视系统而定，可调为 5~50ms
        }
        else if(loopCount<70)
        {
            QThread::msleep(2);
        }
        loopCount=0;
    }
    // 退出前一次性写完所有残余
    CloseFile();
}
void QFileSaveThread::CheckFileExists()
{
    // 定时检查：若文件被删除，则重建同名文件
    if (!m_qsFilePath.isEmpty() && !QFile::exists(m_qsFilePath)) {
        CreatFile(m_qsFilePath);
    }
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
