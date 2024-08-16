#ifndef DBMANIPULATION_H
#define DBMANIPULATION_H
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <qmap.h>
#include <QSqlError>
#include "dbDatas.h"
#include "data.h"


class TimeCondition
{
public:
    TimeCondition(QDateTime _startTime,QDateTime _endTime)
        :startTime(_startTime),endTime(_endTime)
    {
    }
    QDateTime startTime;
    QDateTime endTime;
};

class DbManipulation
{

public:
    static DbManipulation *Get()
    {
        static DbManipulation vt;
        return &vt;
    }
    QMap<int,QString> dbMap;

    DbManipulation();
    ~DbManipulation();
    void initial(QString path);
    QSqlDatabase database;// 用于建立和数据库的连接
    QSqlQuery query;
    bool openDb();
    void closeDb();
    bool isTableExist(QString tableName);
    void createTable(int index);

    bool deleteData(int index, QString id);

    void updateTargetData(int id, const QString &fieldName, const QString &fieldValue);


    bool insertCommandReconTask(QString taskID, int taskType, const QByteArray &taskData);
    bool updateCommandReconTask(QString taskID, int newTaskType, const QByteArray &newTaskData);
    QByteArray fetchCommandReconTaskData(QString taskID);

private:
    bool isDbInited=false;


};

#endif // DBMANIPULATION_H
