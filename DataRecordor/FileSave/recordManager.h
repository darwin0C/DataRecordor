#ifndef RECORDTHREAD_H
#define RECORDTHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QDir>
#include <QProcess>
#include <QDateTime>
#include <QTextStream>
#include <QTimer>
#include "FileSave/FileSaveData.h"
#include "data.h"
#include <QVector>


const int diskMinFree=200*1024;

class RecordManager : public QThread
{
    Q_OBJECT

    QMutex mutex,fileMutex;

    QTimer *checkTmr;
    QTimer    *existTmr;            // ◆ 新增：定时器
    QTextStream* txtOutput;

    QString currentDate;
    QString currentTime;
    QString revDate="",revTime="";
    QString gCurrentfileName="";

    bool isTimeSet=false;

    void checkSize(const QString &result);
    void delOldestFile();
    void getAllFileName(QString path, QVector<QString> &path_vec);
    void delAllFiles();
    QByteArray HexStringToByteArray(QString HexString);
    void creatNewFile(QString date, QString time);
    void newfile(QString date, QString time);

    void SetSysTime(QString date, QString time);
    void checkTime(QString date, QString hour);
    QString findOldestFile() const;
public:
    QString getRecordData(const SerialDataRev &dataRev);
    RecordManager();
    int diskUsed;
    int diskAll;
    int diskUsedPercent;
    int diskFree;
    bool isSDCardOK=true;

    ~RecordManager();
signals:
    void creatFileSig(QString);

private slots:

    void onCheckDisk();
    void onCheckFileExists();      // ◆ 新增：定时检查文件存在性
};

#endif // RECORDTHREAD_H
