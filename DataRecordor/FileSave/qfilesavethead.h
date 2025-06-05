#ifndef QFILESAVETHEAD_H
#define QFILESAVETHEAD_H
#include <QFile>
#include <QMutex>
#include <QThread>
#include <qsemaphore.h>
#include "FileSave/FileSaveData.h"
#include "FileSave/recordManager.h"
#include <QTimer>
#include "data.h"
#include "qtcqueue.h"
#include <atomic>

class QFileSaveThread : public QThread
{
    Q_OBJECT
    std::atomic<quint64> prodBytes  {0};   // 总生产字节
    std::atomic<quint64> consBytes  {0};   // 总消费字节
    std::atomic<int>     ringUsage  {0};   // 上一次打印时 ring 已用
public:
    explicit QFileSaveThread(QObject *parent = NULL);
    ~QFileSaveThread();

public:
    volatile bool m_bStop;
    QMutex m_mutex;
    QMutex m_mutexQueue;

    QFile m_file;     //存储文件

    bool m_bOpen;
    QString m_qsFilePath;

private:
    RecordManager *recordManager=nullptr ;
    char *writeBuffer;
    // 大缓存区
    QMutex    m_mutexOverflow;
    //QByteArray m_overflow;
    double cpuUsedpercent=0;
    QTimer *m_flushTimer;
    QByteArray packSerial(const SerialDataRev &serialData);
public:
    bool CreatFile(QString qsFilePath); //打开文件
    void CloseFile();                        //关闭写文件

    void startRecord();
    void stopRecord();
    int diskUsedPercent();
    int diskRemains();
    bool sdCardStat();
    void onRevCpuinfo(double usedPer);
public slots:
    void revCANData(CanDataBody canData);
    void delAllFiles();

protected:
    virtual void run() override;


private slots:

    void onCreatNewFile(QString fileName);

    void onFlushTimeout();
};

#endif // QFILESAVETHEAD_H
