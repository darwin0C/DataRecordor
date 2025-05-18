
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
    //    m_pBuffer = new byte[m_nCacheSize];  // 预分配 1MB 缓冲
    //    m_nWritePos = 0;
    m_bOpen = false;

    qRegisterMetaType<CanDataBody>("CanDataBody");//自定义类型需要先注册
    connect(&recordManager,&RecordManager::creatFileSig,this,&QFileSaveThead::onCreatNewFile);
    // 每100ms 将大缓存数据搬入小缓冲
    m_ring=new QTCQueue(m_nCacheSize);
    // —— 预分配大缓存区，避免 m_overflow 频繁重分配 ——
    const int BIG_CACHE_SIZE = 50 * 1024 * 1024;  // 50 MB
    m_overflow.reserve(BIG_CACHE_SIZE);
    writeBuffer=new  char[CHUNK];
}

QFileSaveThead::~QFileSaveThead()
{

    stopRecord();       // 停止定时器并发信号
    wait();             // 等待 run() 自然退出
    CloseFile();
    //delete[] m_pBuffer;
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
void QFileSaveThead::revOrigenDataSig(QByteArray data)
{
    if (m_bStop) return;
    data.append('\r');
    data.append('\n');
    // 1) 先将 overflow 里的旧数据尽量塞回 ring
    {
        QMutexLocker lock(&m_mutexOverflow);
        while (!m_overflow.isEmpty() && m_ring->FreeCount() > 0) {
            // 按块移动
            int chunk = qMin<int>(m_overflow.size(),
                                  m_ring->FreeCount());
            m_ring->Add(m_overflow.data(), chunk);
            m_usedSpace.release();
            m_overflow.remove(0, chunk);
        }
    }
    // 2) 再塞入本次 data；若 ring 满，则打入 overflow
    if (m_ring->FreeCount() >= data.size()) {
        m_ring->Add(data.data(), data.size());
        m_usedSpace.release();
    } else {
        QMutexLocker lock(&m_mutexOverflow);
        m_overflow.append(data);
    }
}
void QFileSaveThead::onRevCpuinfo(double usedPer)
{
    cpuUsedpercent=usedPer;
    if(cpuUsedpercent>50)
        qDebug()<<"CPU USED Percent :"<<cpuUsedpercent;
}

void QFileSaveThead::revSerialData(SerialDataRev serialData)
{
    if (m_bStop) return;
    static int datacout=0;
    datacout++;
    if(datacout%1000==0)
        qDebug() << "revSerialData=========================="<<datacout;

    QByteArray data = recordManager.getRecordData(serialData).toLocal8Bit();
    data.append('\n');

    // 1) 先将 overflow 里的旧数据尽量塞回 ring
    {
        QMutexLocker lock(&m_mutexOverflow);
        while (!m_overflow.isEmpty() && m_ring->FreeCount() > 0) {
            // 按块移动
            int chunk = qMin<int>(m_overflow.size(),
                                  m_ring->FreeCount());
            m_ring->Add(m_overflow.data(), chunk);
            m_usedSpace.release();
            m_overflow.remove(0, chunk);
        }
    }

    // 2) 再塞入本次 data；若 ring 满，则打入 overflow
    if (m_ring->FreeCount() >= data.size()) {
        m_ring->Add(data.data(), data.size());
        m_usedSpace.release();
    } else {
        QMutexLocker lock(&m_mutexOverflow);
        m_overflow.append(data);
    }
}

void QFileSaveThead::revCANData(CanDataBody canData)
{
    if(!m_bStop)
    {
        //QString strData=recordManager.getRecordData(canData);
        //saveStringData(strData);
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
    if (m_bOpen && m_file.isOpen()) {
        // 1) 将环形缓冲[m_ring]中的所有数据写入文件
        const int CHUNK = 128 * 1024;
        char tmp[CHUNK];
        while (m_ring->InUseCount() > 0) {
            int got = m_ring->Get(tmp, CHUNK);
            if (got > 0) {
                QMutexLocker lock(&m_mutex);
                m_file.write(tmp, got);
            }
        }
        // 2) 将溢出缓冲[m_overflow]中的剩余数据写入文件
        {
            QMutexLocker ovLock(&m_mutexOverflow);
            if (!m_overflow.isEmpty()) {
                QMutexLocker lock(&m_mutex);
                m_file.write(m_overflow.constData(), m_overflow.size());
                m_overflow.clear();
            }
        }
        // 3) 最后 flush 并关闭
        m_file.flush();
        m_file.close();
        m_bOpen = false;
        qDebug() << "[FileSave] CloseFile: all buffers written and file closed.";
    }
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

        // 2) ring 空时先搬 overflow
        if (m_ring->InUseCount() == 0 && !m_overflow.isEmpty()) {
            QMutexLocker lock(&m_mutexOverflow);
            int toMove = qMin(m_overflow.size(), m_ring->FreeCount());
            m_ring->Add(m_overflow.data(), toMove);
            m_overflow.remove(0, toMove);
            m_usedSpace.release();      // 唤醒下一轮写
        }
        // 3) flush 可改为每写 ≥1MB 或者定时 flush，这里改为写完再 flush
        //        if (m_file.isOpen()) {
        //            m_file.flush();
        //        }
        // 时间到达后再做 flush
        if( loopCounter++>100)
        {
            loopCounter=0;
            qint64 now = timer.elapsed();
            if (now - lastFlush >= FLUSH_INTERVAL_MS) {
                QMutexLocker lock(&m_mutex);
                if (m_file.isOpen())
                    m_file.flush();
                lastFlush = now;
            }
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
