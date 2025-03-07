
#include "qfilesavethead.h"
#include <QDebug>
#include <QTimer>
#include "MsgSignals.h"

QFileSaveThead::QFileSaveThead(QObject *parent)
    : QThread(parent)
    , m_freeSpace(100)
{
    m_bStop = false;
    m_nCacheSize = 1 * 1024 * 1024; //1MB
    m_pBuffer = NULL;
    m_nWritePos = 0;
    m_bOpen = false;


    savTmr=new QTimer(this);
    connect(savTmr,SIGNAL(timeout()),this,SLOT(onTimeSaveFile()));

    qRegisterMetaType<CanDataBody>("CanDataBody");//自定义类型需要先注册
    connect(&recordManager,&RecordManager::creatFileSig,this,&QFileSaveThead::onCreatNewFile);
}

QFileSaveThead::~QFileSaveThead()
{
    m_mutexQueue.lock(); //进入临界区
    m_bStop = true;
    m_usedSpace.release();
    this->wait();
    m_mutexQueue.unlock(); //离开临界区
    CloseFile();
    ClearBuf();
}

void QFileSaveThead::startRecord()
{
    m_bStop = false;
    start(QThread::TimeCriticalPriority);
    savTmr->start(2000);
    qDebug()<<"save file thread "<<isRunning();
}

void QFileSaveThead::stopRecord()
{
    m_bStop = true;
    savTmr->stop();
}

void QFileSaveThead::onCreatNewFile(QString fileName)
{
    CloseFile();
    CreatFile(fileName);
}

void QFileSaveThead::revSerialData(SerialDataRev serialData)
{
    if(!m_bStop)
    {
        //qDebug() << "revSerialData==========================";
        QString strData=recordManager.getRecordData(serialData);
        saveStringData(strData);
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

void QFileSaveThead::saveStringData(const QString &data)
{
    WriteFile((unsigned char *)data.toLocal8Bit().data(),data.length());
}

void QFileSaveThead::WriteFile(byte *pBuffer, int nLen)
{
    QMutexLocker locker(&m_mutex);
    if (!m_file.isOpen())
        return;
    if (m_pBuffer == NULL) {
        m_pBuffer = new byte[m_nCacheSize];
        m_nWritePos = 0;
    }
    //   qDebug()<<"buuff1";
    if (m_nWritePos + nLen < m_nCacheSize) {
        memcpy(m_pBuffer + m_nWritePos, pBuffer, nLen);
        //      qDebug()<<"buuff2";
        m_nWritePos += nLen;
    }
    else //写入数据
    {
        bool bCopy = false;
        if (m_nWritePos + nLen == m_nCacheSize) {
            memcpy(m_pBuffer + m_nWritePos, pBuffer, nLen);
            m_nWritePos += nLen;
            bCopy = true;
        }
        TDataBuffer tDataBuffer;
        tDataBuffer.pBuffer = m_pBuffer;
        tDataBuffer.nLen = m_nWritePos;
        if (m_freeSpace.tryAcquire()) {
            m_mutexQueue.lock(); //进入临界区
            m_queueDataBuffer.append(tDataBuffer);
            m_mutexQueue.unlock(); //离开临界区
            m_usedSpace.release(); //消费锁释放
        } else {
            // qDebug()<< "----------The storage file queue is too long to be reproduced!----------\r";
        }
        m_pBuffer = new byte[m_nCacheSize];
        //缓存不够拷贝，将本包拷贝至下一次缓冲区
        if (!bCopy) {
            memcpy(m_pBuffer, pBuffer, nLen);
            m_nWritePos = nLen;
        } else
            m_nWritePos = 0;
    }
}

void QFileSaveThead::CloseFile()
{
    m_bOpen = false;
    m_mutexQueue.lock(); //进入临界区
    while (!m_queueDataBuffer.empty()) {
        TDataBuffer tDataBuffer = m_queueDataBuffer.dequeue();
        if (tDataBuffer.pBuffer != NULL) {
            if (m_file.isOpen())
                m_file.write((char *) tDataBuffer.pBuffer, tDataBuffer.nLen);
            delete[] tDataBuffer.pBuffer;
        }
    }
    m_mutexQueue.unlock(); //离开临界区
    QMutexLocker locker(&m_mutex);
    if (m_pBuffer != NULL) {
        if (m_file.isOpen())
            m_file.write((char *) m_pBuffer, m_nWritePos);
        delete[] m_pBuffer;
        m_pBuffer = NULL;
        m_nWritePos = 0;
    }
    if (m_file.isOpen()) {
        if (m_file.size() == 0)
            m_file.remove();
        m_file.close();
    }
}

void QFileSaveThead::ClearBuf()
{
    QMutexLocker locker(&m_mutex);
    while (!m_queueDataBuffer.empty()) {
        TDataBuffer tDataBuffer = m_queueDataBuffer.dequeue();
        if (tDataBuffer.pBuffer != NULL)
            delete[] tDataBuffer.pBuffer;
    }
    if (m_pBuffer != NULL)
        delete[] m_pBuffer;
}

bool QFileSaveThead::CreatFile(QString qsFilePath)
{
    m_qsFilePath = qsFilePath;
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
    while (!m_bStop) {
        m_usedSpace.acquire();
        if (m_bStop)
            return;
        bool sigSend=false;
        while (!m_bStop && !m_queueDataBuffer.empty()) {
            if(!sigSend)
            {
                emit MsgSignals::getInstance()->sendLEDStatSig();
                sigSend = true;
            }
            m_mutexQueue.lock(); //进入临界区
            TDataBuffer tDataBuffer = m_queueDataBuffer.dequeue();
            m_mutexQueue.unlock(); //离开临界区
            m_freeSpace.release();

            //处理数据
            if (tDataBuffer.pBuffer != NULL) {
                if (m_file.isOpen())
                {
                    m_file.write((char *) tDataBuffer.pBuffer, tDataBuffer.nLen);
                    m_file.flush();
                }
                // delete[] tDataBuffer.pBuffer;
            }
        }
    }
}
void QFileSaveThead::onTimeSaveFile()
{
    if (m_pBuffer != NULL&&m_nWritePos>0) {
        m_mutexQueue.lock(); //进入临界区
        TDataBuffer tDataBuffer;
        tDataBuffer.pBuffer = m_pBuffer;
        tDataBuffer.nLen = m_nWritePos;
        if (m_freeSpace.tryAcquire()) {

            m_queueDataBuffer.append(tDataBuffer);
            m_nWritePos = 0;
            m_usedSpace.release(); //消费锁释放
        }
        m_mutexQueue.unlock(); //离开临界区
    }
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
