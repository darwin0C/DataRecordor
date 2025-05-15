#ifndef QFILESAVETHEAD_H
#define QFILESAVETHEAD_H
#include <QFile>
#include <QMutex>
#include <QQueue>
#include <QThread>
#include <qsemaphore.h>
#include "FileSave/FileSaveData.h"
#include "FileSave/recordManager.h"
#include <QTimer>
#include "data.h"
#include "qtcqueue.h"
class QFileSaveThead : public QThread
{
    Q_OBJECT
public:
    explicit QFileSaveThead(QObject *parent = NULL);
    ~QFileSaveThead();

public:
    volatile bool m_bStop;
    QMutex m_mutex;
    QMutex m_mutexQueue;

    //    QSemaphore m_freeSpace;
    //    QSemaphore m_usedSpace;
    //    QQueue<TDataBuffer> m_queueDataBuffer;

    // 用 QTCQueue 代替原来的队列与堆缓冲
    QTCQueue  *m_ring;          // 单一环形缓冲区
    QSemaphore m_usedSpace;     // 信号量：写线程等待 ring 有数据


    QFile m_file;     //存储文件
    int m_nCacheSize; //缓存大小默认1MB
    //小缓冲不再需要，改为临时栈缓冲
    //    byte *m_pBuffer;  //缓存数据
    //    int m_nWritePos;  //写入位置

    bool m_bOpen;
    QString m_qsFilePath;

private:
    RecordManager recordManager;
    QTimer *savTmr=NULL;
    // 大缓存区
    QMutex    m_mutexOverflow;
    QByteArray m_overflow;

public:
    bool CreatFile(QString qsFilePath); //打开文件
    int GetCacheSize() const;
    void SetCacheSize(int nCacheSize);
    void CloseFile();                        //关闭写文件

    void startRecord();
    void stopRecord();
    int diskUsedPercent();
    int diskRemains();
    bool sdCardStat();
public slots:
    void revCANData(CanDataBody canData);
    void revSerialData(SerialDataRev serialData);
    void delAllFiles();
protected:
    virtual void run() override;
private slots:
    void onTimeSaveFile();
    void onCreatNewFile(QString fileName);

};

#endif // QFILESAVETHEAD_H
