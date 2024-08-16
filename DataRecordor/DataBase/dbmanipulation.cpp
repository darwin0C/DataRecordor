#include "dbmanipulation.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <QCoreApplication>

const QString dataTimeStyle="yyyy-MM-dd HH:mm:ss";//"yyyy-MM-dd HH:mm:ss.zzz"
DbManipulation::DbManipulation()
{
    if(!isDbInited)
    {
        QString fileName=QCoreApplication::applicationDirPath()+DBDATEFileName;
        QFileInfo file(fileName);
        if(!file.absoluteDir().exists())
        {
            QDir dir;
            dir.mkdir(file.absoluteDir().path());
        }
        initial(fileName);
        isDbInited=true;
    }
}
DbManipulation::~DbManipulation()
{
    closeDb();
}
//初始化数据库
void DbManipulation::initial(QString path){
    if(QSqlDatabase::contains("qt_sql_default_connection"))
        database = QSqlDatabase::database("qt_sql_default_connection");
    else
        database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName(path);
    dbMap.clear();
    dbMap[DB_target_data]=("target_data");
    dbMap[DB_reconTask_data]=("reconTask_data");
    dbMap[DB_Equ_Ability]=("Equ_Ability");
    dbMap[DB_Equ_WorkStat]=("Equ_WorkStat");
    dbMap[DB_TaskReconPoints]=("TaskReconPoints");
    dbMap[DB_TaskStat]=("TaskStat");//记录任务执行过程的状态
    dbMap[DB_CommandTask]=("CommandReconTask");//指挥下发任务
    if(openDb())
    {
        for(auto it=dbMap.begin();it!=dbMap.end();++it)
        {
            createTable(it.key());
        }
        qDebug()<<"initial success!";
    }
}
//打开数据库
bool DbManipulation::openDb(){
    if(database.isOpen())
        return true;
    if (!database.open())
    {
        qDebug() << "Error: Failed to connect database.";
        return false;
    }else {
        return true;
    }
}

//关闭数据库
void DbManipulation::closeDb(){
    database.close();
}

//判断表格是否存在
bool DbManipulation::isTableExist(QString tableName){
    QStringList list=database.tables();
    return database.tables().contains(tableName);
}

//创建数据表
void DbManipulation::createTable(int index)
{
    QString createCmd="";
    QSqlQuery query(database);
    if(isTableExist(dbMap[index])){
        return;
    }
    switch (index) {
    case DB_target_data:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1( "
                            "time varchar(50),taskId varchar(50) ,targetId varchar(50) PRIMARY KEY,"
                            "TargetType varchar(50),reconSource varchar(50),"
                            "posType varchar(50),X REAL, Y REAL, H REAL,  "
                            "A REAL,E REAL, R REAL, P REAL,N0 REAL,"
                            "beta REAL,epsilon REAL, distance REAL,"
                            "ImgName varchar(50));").arg(dbMap[DB_target_data]);
        break;
    case DB_reconTask_data:
        createCmd =QString ("CREATE TABLE IF NOT EXISTS %1( "
                            "taskId varchar(50) PRIMARY KEY,"
                            "mainTaskId VARCHAR(50), "
                            "beginTime varchar(50), lastTime INTEGER, "
                            "executionUnit INTEGER,taskType INTEGER,"
                            "taskStat INTEGER);").arg(dbMap[DB_reconTask_data]);
        break;

    case DB_Equ_Ability:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "EquType INTEGER,"
                            "ccdEnabled INTEGER,  ccdMaxViewField REAL,  ccdMaxReconDistance REAL,"
                            "irEnabled INTEGER,  irMaxViewField REAL,  irMaxReconDistance REAL,"
                            "rangefinderEnabled INTEGER,  rangefinderMaxRangeDistance REAL, "
                            "irradiatorEnabled INTEGER,  irradiatorMaxRangeDistance REAL, "
                            "servEnabled INTEGER,  azMaxAngleRange REAL,  azMinAngleRange REAL, "
                            "elMaxAngleRange REAL,elMinAngleRange REAL,"
                            "AngleMeasurAccuracy REAL,turnAroundTime REAL,"
                            "isBattery INTEGER, maxBatteryLift REAL);").arg(dbMap[DB_Equ_Ability]);

        break;

    case DB_Equ_WorkStat:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "time varchar(50),"
                            "EquType INTEGER, EquStat INTEGER, "
                            "workFlag INTEGER, errorCode varchar(50));").arg(dbMap[DB_Equ_WorkStat]);
        break;
    case DB_TaskReconPoints://区域侦察任务/目标侦察任务
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "taskId varchar(50),pointNum INTEGER, "
                            "posType varchar(50),X REAL, Y REAL, H REAL,"
                            "FOREIGN KEY (taskId) REFERENCES %2 (taskId) ON DELETE CASCADE);")
                .arg(dbMap[DB_TaskReconPoints])
                .arg(dbMap[DB_reconTask_data]);
        break;

    case DB_TaskStat://侦察任务执行状态
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "taskId varchar(50),"
                            "time varchar(50), "
                            "taskStat INTEGER,"
                            "FOREIGN KEY (taskId) REFERENCES %2 (taskId) ON DELETE CASCADE);")
                .arg(dbMap[DB_TaskStat])
                .arg(dbMap[DB_reconTask_data]);
        break;
    case DB_CommandTask:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1 "
                            "(taskId VARCHAR(50) PRIMARY KEY, "
                            "taskType INTEGER, "
                            "taskData BLOB, "
                            "FOREIGN KEY (taskId) REFERENCES %2(taskId) ON DELETE CASCADE);")
                .arg(dbMap[DB_CommandTask])
                .arg(dbMap[DB_reconTask_data]);
        break;
    default:
        break;
    }
    if(createCmd!="")
        if(!query.exec(createCmd))
        {
            qDebug()<<"creat table failed:"<<dbMap[index]<<query.lastError();
        }
}
//表格内插入数据
//target_data表格插入数据
/******************************************************************
 *数据库操作
 *插入数据
*****************************************************************/

////插入侦察区域数据
//bool DbManipulation:: insertTaskReconPointsData(const ITask_ReconData &Recontask) {
//    QString tableName=dbMap[DB_TaskReconPoints];
//    if(openDb()&&isTableExist(tableName))
//    {
//        QSqlQuery query;
//        foreach(IXYHCoorData coorData,Recontask.taskPoints.multiPointCoor)
//        {
//            query.prepare(QString("INSERT INTO %1 (taskId, pointNum, posType ,X , Y , H )"
//                                  "VALUES (:taskId, :pointNum, :posType ,:X , :Y , :H)").arg(tableName));
//            query.bindValue(":taskId", Recontask.taskData.taskId);
//            query.bindValue(":pointNum", Recontask.taskPoints.pointNum);
//            query.bindValue(":posType", coorData.type);
//            query.bindValue(":X", coorData.x);
//            query.bindValue(":Y", coorData.y);
//            query.bindValue(":H", coorData.h);
//            // 执行插入
//            if (!query.exec()) {
//                qDebug() << "Error inserting data:" << query.lastError();
//                return false;
//            } else {
//                qDebug() << "Data inserted successfully!";
//            }
//        }
//    }
//    return true;
//}



bool DbManipulation::insertCommandReconTask(QString taskID, int taskType, const QByteArray &taskData) {
    QString tableName = dbMap[DB_CommandTask];
    qDebug() << "Is table exist: " << isTableExist(tableName);
    qDebug() << "Table name: " << tableName;
    qDebug() << "Open database: " << openDb();


    if (openDb() && isTableExist(tableName))
    {
        QSqlQuery query;
        query.prepare(QString("INSERT INTO %1 (taskId, taskType, taskData) "
                              "VALUES (:taskId, :taskType, :taskData)").arg(tableName));

        query.bindValue(":taskId", taskID);
        query.bindValue(":taskType", taskType);
        query.bindValue(":taskData", taskData);

        if (!query.exec()) {
            qDebug() << "Error inserting data:" << query.lastError();
            return false;
        } else {
            qDebug() << "Data inserted successfully!";
            return true;
        }
    }
    return false;
}

/******************************************************************
 *数据库操作
 *删除数据
*****************************************************************/
//表格内删除数据
bool DbManipulation::deleteData(int index, QString id){
    bool res=false;
    if(openDb()&&isTableExist(dbMap[index]))
    {
        QSqlQuery query(database);
        QString delcmd;
        switch (index) {
        case DB_target_data:
            delcmd = QString("delete from %1 where targetId='%2';").arg(dbMap[index]).arg(id);
            res=query.exec(delcmd);
            break;
        case DB_reconTask_data:
            delcmd = QString("delete from %1 where taskId='%2';").arg(dbMap[index]).arg(id);
            res=query.exec(delcmd);
            break;
        case DB_Equ_Ability:
            delcmd = QString("delete from %1 where id='%2';").arg(dbMap[index]).arg(id);
            res=query.exec(delcmd);
            break;
        case DB_Equ_WorkStat:
            delcmd = QString("delete from %1 where id='%2';").arg(dbMap[index]).arg(id);
            res=query.exec(delcmd);
            break;
        case DB_TaskReconPoints:
            delcmd = QString("delete from %1 where id='%2';").arg(dbMap[index]).arg(id);
            res=query.exec(delcmd);
            break;
        }
    }
    if(!res)
        qDebug()<<"dele data failed:"<<query.lastError();
    return res;
}
/******************************************************************
 *数据库操作
 *更新数据
*****************************************************************/
//表格target_data更新数据


void DbManipulation::updateTargetData(int id, const QString &fieldName, const QString &fieldValue) {
    QSqlQuery query;
    QString strSQL=QString("UPDATE target_data SET %1 = :fieldValue WHERE id = :id").arg(fieldName);
    query.prepare(strSQL);
    query.bindValue(":fieldValue", fieldValue);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Error updating data:" << query.lastError();
    } else {
        qDebug() << "Updated successfully!";
    }
}


//bool DbManipulation::updateTaskReconPointsData(const ITask_ReconData &Recontask) {
//    QString tableName = dbMap[DB_TaskReconPoints];
//    if (openDb() && isTableExist(tableName)) {
//        QSqlQuery query;

//        // 开始事务
//        QSqlDatabase::database().transaction();

//        // 删除所有相同taskId的数据
//        query.prepare(QString("DELETE FROM %1 WHERE taskId = :taskId").arg(tableName));
//        query.bindValue(":taskId", Recontask.taskData.taskId);
//        if (!query.exec()) {
//            qDebug() << "Error deleting data:" << query.lastError();
//            // 回滚事务
//            QSqlDatabase::database().rollback();
//            return false;
//        }

//        // 插入新数据
//        foreach (IXYHCoorData coorData, Recontask.taskPoints.multiPointCoor) {
//            query.prepare(QString("INSERT INTO %1 (taskId, pointNum, posType ,X , Y , H )"
//                                  "VALUES (:taskId, :pointNum, :posType ,:X , :Y , :H)").arg(tableName));
//            query.bindValue(":taskId", Recontask.taskData.taskId);
//            query.bindValue(":pointNum", Recontask.taskPoints.pointNum);
//            query.bindValue(":posType", coorData.type);
//            query.bindValue(":X", coorData.x);
//            query.bindValue(":Y", coorData.y);
//            query.bindValue(":H", coorData.h);
//            // 执行插入
//            if (!query.exec()) {
//                qDebug() << "Error inserting data:" << query.lastError();
//                // 回滚事务
//                QSqlDatabase::database().rollback();
//                return false;
//            }
//        }
//        // 提交事务
//        QSqlDatabase::database().commit();
//        return true;
//    }
//    return false;
//}

bool DbManipulation::updateCommandReconTask(QString taskID, int newTaskType, const QByteArray &newTaskData) {
    QString tableName = dbMap[DB_CommandTask];
    if (openDb() && isTableExist(tableName)) {
        QSqlQuery query;
        query.prepare(QString("UPDATE %1 SET taskType = :newTaskType, taskData = :newTaskData "
                              "WHERE taskID = :taskID").arg(tableName));

        query.bindValue(":newTaskType", newTaskType);
        query.bindValue(":newTaskData", newTaskData);
        query.bindValue(":taskID", taskID);

        if (!query.exec()) {
            qDebug() << "Error updating data:" << query.lastError();
            return false;
        } else {
            qDebug() << "Data updated successfully!";
            return true;
        }
    }
    return false;
}

/******************************************************************
 *数据库操作
 *查询数据
*****************************************************************/
//查询表格数据


QByteArray DbManipulation::fetchCommandReconTaskData(QString taskID) {
    QByteArray taskData;
    QString tableName = dbMap[DB_CommandTask];
    if (openDb() && isTableExist(tableName)) {
        QSqlQuery query;
        query.prepare(QString("SELECT taskData FROM %1 WHERE taskId = :taskId").arg(tableName));
        query.bindValue(":taskId", taskID);

        if (query.exec() && query.next()) {
            taskData = query.value("taskData").toByteArray();
        } else {
            qDebug() << "Error fetching data:" << query.lastError();
        }
    }
    return taskData;
}

/******************************************************************
 *数据库操作
 *通过时间筛选查询
*****************************************************************/


//QList<ITask_Data> DbManipulation::queryAllReconTaskData(const TimeCondition *timeConditon)
//{
//    QList<ITask_Data> taskDataList;
//    QString tableName = dbMap[DB_reconTask_data];
//    if (openDb() && isTableExist(tableName)) {
//        QSqlQuery query;
//        QString queryString;

//        // 构建查询字符串

//        queryString = QString("SELECT * FROM %1").arg(tableName);

//        // 添加时间过滤条件
//        if (timeConditon && timeConditon->startTime.isValid() && timeConditon->endTime.isValid()) {
//            queryString += " WHERE beginTime BETWEEN :startTime AND :endTime";
//        }
//        queryString += " ORDER BY beginTime DESC;";

//        query.prepare(queryString);

//        if (timeConditon && timeConditon->startTime.isValid() && timeConditon->endTime.isValid()) {
//            query.bindValue(":startTime", timeConditon->startTime.toString(dataTimeStyle));
//            query.bindValue(":endTime", timeConditon->endTime.toString(dataTimeStyle));
//        }
//        // 执行查询并处理结果
//        if (query.exec()) {
//            while (query.next()) {
//                ITask_Data taskData;
//                taskData.taskId = query.value("taskId").toString();
//                taskData.mainTaskID = query.value("mainTaskId").toString();
//                taskData.beginTime = QDateTime::fromString(query.value("beginTime").toString(),dataTimeStyle);
//                taskData.lastTime = query.value("lastTime").toUInt();
//                taskData.executionUnit = static_cast<RECON_EQU_TYPE>(query.value("executionUnit").toInt());
//                taskData.taskType = static_cast<TASK_TYPE>(query.value("taskType").toInt());
//                TASK_STAT taskStatTemp = TaskStat_OtherCases;
//                if (queryTaskLastStat(taskData.taskId, taskStatTemp)) {
//                    taskData.taskStat = taskStatTemp;
//                } else {
//                    taskData.taskStat = static_cast<TASK_STAT>(query.value("taskStat").toInt());
//                }

//                taskDataList.append(taskData);
//            }
//        } else {
//            qDebug() << "Error fetching data:" << query.lastError();
//        }
//        qDebug() << "Prepared query:" << queryString;//：输出准备好的查询字符串。
//        qDebug() << "Bound values:" << query.boundValues();//：输出绑定的参数和值。

//    }
//    return taskDataList;
//}




