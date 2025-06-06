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

    // 用 QTCQueue 代替原来的队列与堆缓冲
    QTCQueue  *m_ring;          // 单一环形缓冲区
    QSemaphore m_usedSpace;     // 信号量：写线程等待 ring 有数据


    QFile m_file;     //存储文件
    int m_nCacheSize; //缓存大小默认1MB

    bool m_bOpen;
    QString m_qsFilePath;

private:
    QTimer timer;
    RecordManager recordManager;
    char *writeBuffer;
    // 大缓存区
    QMutex    m_mutexOverflow;
    //QByteArray m_overflow;
    double cpuUsedpercent=0;
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
    void onRevCpuinfo(double usedPer);
public slots:
    void revSerialData();
    void delAllFiles();

protected:
    virtual void run() override;
private slots:

    void onCreatNewFile(QString fileName);

};

#endif // QFILESAVETHEAD_H
