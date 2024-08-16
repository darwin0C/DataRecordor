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
    QSemaphore m_freeSpace;
    QSemaphore m_usedSpace;
    QQueue<TDataBuffer> m_queueDataBuffer;
    QFile m_file;     //存储文件
    int m_nCacheSize; //缓存大小默认1MB
    byte *m_pBuffer;  //缓存数据
    int m_nWritePos;  //写入位置
    bool m_bOpen;
    QString m_qsFilePath;

private:
    void ClearBuf(void); //清除Buffer
    RecordManager recordManager;
    QTimer *savTmr=NULL;
public:
    bool CreatFile(QString qsFilePath); //打开文件
    int GetCacheSize() const;
    void SetCacheSize(int nCacheSize);
    void WriteFile(byte *pBuffer, int nLen); //写文件
    void CloseFile();                        //关闭写文件

    void saveStringData(const QString &data);
    void startRecord();
    void stopRecord();
    int diskUsedPercent();
    int diskRemains();
    bool sdCardStat();
public slots:
    void revCANData(CanDataBody canData);
    void revSerialData(SerialDataRev serialData);
protected:
    virtual void run() override;
private slots:
    void onTimeSaveFile();
    void onCreatNewFile(QString fileName);
};

#endif // QFILESAVETHEAD_H
