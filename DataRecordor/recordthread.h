#ifndef RECORDTHREAD_H
#define RECORDTHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QDir>
#include <QProcess>
#include <QDateTime>
#include <QTextStream>
const QString gPath="/run/media/mmcblk0p1/data/";
const int diskMinFree=200*1024;

class RecordThread : public QThread
{
    Q_OBJECT
    void WriteToFile();
    QString canDataRev;
    QMutex mutex,fileMutex;
    QString dataToWrite;
    QFile* file;
    QProcess *process;
    bool isSDCardOK;
    QTextStream* txtOutput;

    QString currentDate;
    QString currentTime;
    QString revDate,revTime;
    QString gCurrentfileName="";
    bool isCreatingFile=false;

    int diskUsed;
    int diskAll;
    int diskUsedPercent;
    int diskFree;
    bool isTimeSet;

    void timerProcessSerialData();
    void readDiskData();
    void checkSize(const QString &result);
    void delOldestFile();
    void getAllFileName(QString path, QVector<QString> &path_vec);
    void delAllFiles();
    QByteArray HexStringToByteArray(QString HexString);
    void creatNewFile(QString date, QString time);
    void newfile(QString date, QString time);
    QString getRecordData(QString dataRev);
    bool checkData(QString dataRev);
    int getCheckNum(QString data);
    void SetSysTime(QString date, QString time);
public:
    RecordThread();
};

#endif // RECORDTHREAD_H
