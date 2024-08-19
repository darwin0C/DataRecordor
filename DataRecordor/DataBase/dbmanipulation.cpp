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

    dbMap[DB_Equ_Name]=("Equ_Name");
    dbMap[DB_Equ_WorkStat]=("Equ_WorkStat");
    dbMap[DB_Equ_ErrorInfo]=("Equ_ErrorInfo");
    dbMap[DB_Equ_TotalWorkTime]=("Equ_TotalWorkTime");
    dbMap[DB_AlarmInfo]=("AlarmInfo");
    dbMap[DB_GunMoveInfo]=("GunMoveInfo");
    dbMap[DB_GunShootInfo]=("GunShootInfo");


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
    case DB_Equ_Name:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1 ("
                            "device_id INTEGER PRIMARY KEY, "
                            "device_name varchar(50) NOT NULL)").arg(dbMap[DB_Equ_Name]);
        break;

    case DB_Equ_WorkStat:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1 ("
                            "stat_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "device_id INTEGER NOT NULL, "
                            "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                            "status varchar(50) NOT NULL, "
                            "error_code BLOB, "
                            "FOREIGN KEY (device_id) REFERENCES %2(device_id))")
                .arg(dbMap[DB_Equ_WorkStat])
                .arg(dbMap[DB_Equ_Name]);
        break;

    case DB_Equ_ErrorInfo:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "stat_id INTEGER NOT NULL, "
                            "error_info varchar(50), "
                            "FOREIGN KEY (stat_id) REFERENCES %2 (stat_id) ON DELETE CASCADE);")
                .arg(dbMap[DB_Equ_ErrorInfo])
                .arg(dbMap[DB_Equ_WorkStat]);
        break;
    case DB_Equ_TotalWorkTime:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "device_id INTEGER NOT NULL, "
                            "total_worktime INTEGER NOT NULL, "
                            "FOREIGN KEY (device_id) REFERENCES %2 (device_id) ON DELETE CASCADE);")
                .arg(dbMap[DB_Equ_TotalWorkTime])
                .arg(dbMap[DB_Equ_Name]);
        break;
    case DB_AlarmInfo:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1 ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "alarm_content TEXT NOT NULL, "
                            "alarm_status_change_time DATETIME DEFAULT CURRENT_TIMESTAMP, "
                            "alarm_info TEXT)")
                .arg(dbMap[DB_AlarmInfo]);
        break;
    case DB_GunMoveInfo:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1 ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "barrel_direction REAL NOT NULL, "
                            "elevation_angle REAL NOT NULL, "
                            "chassis_roll_angle REAL NOT NULL, "
                            "chassis_pitch_angle REAL NOT NULL, "
                            "status_change_time DATETIME DEFAULT CURRENT_TIMESTAMP, "
                            "auto_move_status INTEGER NOT NULL)")
                .arg(dbMap[DB_GunMoveInfo]);
        break;
    case DB_GunShootInfo:
        createCmd = QString( "CREATE TABLE IF NOT EXISTS %1 ("
                             "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                             "barrel_direction REAL NOT NULL, "
                             "elevation_angle REAL NOT NULL, "
                             "chassis_roll_angle REAL NOT NULL, "
                             "chassis_pitch_angle REAL NOT NULL, "
                             "status_change_time DATETIME DEFAULT CURRENT_TIMESTAMP, "
                             "firing_signal_complete INTEGER NOT NULL, "
                             "recoil_status INTEGER NOT NULL, "
                             "muzzle_velocity_valid INTEGER NOT NULL, "
                             "charge_temperature REAL, "
                             "muzzle_velocity REAL)")
                .arg(dbMap[DB_GunShootInfo]);
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

/******************************************************************
 *数据库操作
 *插入数据
*****************************************************************/
bool DbManipulation::insertDeviceName(const DeviceName& deviceName) {
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (device_id, device_name) VALUES (:device_id, :device_name)").arg(dbMap[DB_Equ_Name]));
    query.bindValue(":device_id", deviceName.deviceAddress);
    query.bindValue(":device_name", deviceName.deviceName);
    return query.exec();
}


bool DbManipulation::insertDeviceStatusInfo(const DeviceStatusInfo& statusInfo) {
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (device_id, timestamp, status, error_code) VALUES (:device_id, :timestamp, :status, :error_code)")
                  .arg(dbMap[DB_Equ_WorkStat]));

    query.bindValue(":device_id", statusInfo.deviceStatus.deviceAddress);

    // 将 LongDateTime 转换为 QDateTime
    QDateTime timestamp(TimeFormatTrans::TimeFormatTrans::getDateTime(statusInfo.dateTime));
    query.bindValue(":timestamp", timestamp);

    query.bindValue(":status", statusInfo.deviceStatus.Status);

    // 将故障信息数组转换为 QByteArray
    query.bindValue(":error_code", QByteArray(reinterpret_cast<const char*>(statusInfo.deviceStatus.faultInfo), sizeof(statusInfo.deviceStatus.faultInfo)));

    return query.exec();
}

bool DbManipulation::insertDeviceErrorInfo(const DeviceErrorInfo& errorInfo) {
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (stat_id, error_info) VALUES (:stat_id, :error_info)").arg(dbMap[DB_Equ_ErrorInfo]));
    query.bindValue(":stat_id", errorInfo.statId);
    query.bindValue(":error_info", errorInfo.errorInfo);
    return query.exec();
}

bool DbManipulation::insertDeviceTotalWorkTime(const DeviceTotalWorkTime& workTimeInfo) {
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (device_id, total_worktime) VALUES (:device_id, :total_worktime)")
                  .arg(dbMap[DB_Equ_TotalWorkTime]));

    query.bindValue(":device_id", workTimeInfo.deviceId);
    query.bindValue(":total_worktime", workTimeInfo.totalWorkTime);

    return query.exec();
}

bool DbManipulation::insertAlarmInfo(const AlarmInfo& alarmInfo) {
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (alarmContent, statusChangeTime, alarmDetails) VALUES (:alarmContent, :statusChangeTime, :alarmDetails)").arg(dbMap[DB_AlarmInfo]));
    query.bindValue(":alarmContent", alarmInfo.alarmContent);
    query.bindValue(":statusChangeTime", QDateTime(TimeFormatTrans::TimeFormatTrans::getDateTime(alarmInfo.statusChangeTime)));
    query.bindValue(":alarmDetails", alarmInfo.alarmDetails);
    return query.exec();
}

bool DbManipulation::insertGunMoveData(const GunMoveData& gunMoveData) {
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (barrelDirection, elevationAngle, chassisRoll, chassisPitch, statusChangeTime, autoAdjustmentStatus) "
                          "VALUES (:barrelDirection, :elevationAngle, :chassisRoll, :chassisPitch, :statusChangeTime, :autoAdjustmentStatus)").arg(dbMap[DB_GunMoveInfo]));
    query.bindValue(":barrelDirection", gunMoveData.barrelDirection);
    query.bindValue(":elevationAngle", gunMoveData.elevationAngle);
    query.bindValue(":chassisRoll", gunMoveData.chassisRoll);
    query.bindValue(":chassisPitch", gunMoveData.chassisPitch);
    query.bindValue(":statusChangeTime", QDateTime(TimeFormatTrans::TimeFormatTrans::getDateTime(gunMoveData.statusChangeTime)));
    query.bindValue(":autoAdjustmentStatus", gunMoveData.autoAdjustmentStatus);
    return query.exec();
}

bool DbManipulation::insertGunFiringData(const GunFiringData& gunFiringData) {
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (barrelDirection, elevationAngle, chassisRoll, chassisPitch, statusChangeTime, firingCompletedSignal, recoilStatus, muzzleVelocityValid, propellantTemperature, muzzleVelocity) "
                          "VALUES (:barrelDirection, :elevationAngle, :chassisRoll, :chassisPitch, :statusChangeTime, :firingCompletedSignal, :recoilStatus, :muzzleVelocityValid, :propellantTemperature, :muzzleVelocity)").arg(dbMap[DB_GunShootInfo]));
    query.bindValue(":barrelDirection", gunFiringData.barrelDirection);
    query.bindValue(":elevationAngle", gunFiringData.elevationAngle);
    query.bindValue(":chassisRoll", gunFiringData.chassisRoll);
    query.bindValue(":chassisPitch", gunFiringData.chassisPitch);
    query.bindValue(":statusChangeTime", QDateTime(TimeFormatTrans::TimeFormatTrans::getDateTime(gunFiringData.statusChangeTime)));
    query.bindValue(":firingCompletedSignal", gunFiringData.firingCompletedSignal);
    query.bindValue(":recoilStatus", gunFiringData.recoilStatus);
    query.bindValue(":muzzleVelocityValid", gunFiringData.muzzleVelocityValid);
    query.bindValue(":propellantTemperature", gunFiringData.propellantTemperature);
    query.bindValue(":muzzleVelocity", gunFiringData.muzzleVelocity);
    return query.exec();
}



/******************************************************************
 *数据库操作
 *更新数据
*****************************************************************/
bool DbManipulation::updateDeviceStatusInfo(const DeviceStatusInfo& statusInfo) {
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET timestamp = :timestamp, status = :status, error_code = :error_code WHERE device_id = :device_id")
                  .arg(dbMap[DB_Equ_WorkStat]));

    query.bindValue(":device_id", statusInfo.deviceStatus.deviceAddress);

    // 将 LongDateTime 转换为 QDateTime
    query.bindValue(":timestamp", TimeFormatTrans::TimeFormatTrans::getDateTime(statusInfo.dateTime));
    query.bindValue(":status", statusInfo.deviceStatus.Status);
    // 将故障信息数组转换为 QByteArray
    query.bindValue(":error_code", QByteArray(reinterpret_cast<const char*>(statusInfo.deviceStatus.faultInfo), sizeof(statusInfo.deviceStatus.faultInfo)));
    return query.exec();
}
bool DbManipulation::updateDeviceErrorInfo(const DeviceErrorInfo& errorInfo) {
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET error_info = :error_info WHERE stat_id = :stat_id")
                  .arg(dbMap[DB_Equ_ErrorInfo]));

    query.bindValue(":error_info", errorInfo.errorInfo);
    query.bindValue(":stat_id", errorInfo.statId);

    return query.exec();
}

bool DbManipulation::updateDeviceTotalWorkTime(const DeviceTotalWorkTime& workTimeInfo) {
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET total_worktime = :total_worktime WHERE device_id = :device_id")
                  .arg(dbMap[DB_Equ_TotalWorkTime]));

    query.bindValue(":total_worktime", workTimeInfo.totalWorkTime);
    query.bindValue(":device_id", workTimeInfo.deviceId);

    return query.exec();
}


bool DbManipulation::updateAlarmInfo(const AlarmInfo& alarmInfo) {
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET statusChangeTime = :statusChangeTime, alarmDetails = :alarmDetails WHERE alarmContent = :alarmContent")
                  .arg(dbMap[DB_AlarmInfo]));

    query.bindValue(":statusChangeTime", QDateTime(TimeFormatTrans::TimeFormatTrans::getDateTime(alarmInfo.statusChangeTime)));
    query.bindValue(":alarmDetails", alarmInfo.alarmDetails);
    query.bindValue(":alarmContent", alarmInfo.alarmContent);

    return query.exec();
}


bool DbManipulation::updateGunMoveData(const GunMoveData& gunMoveData) {
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET elevationAngle = :elevationAngle, chassisRoll = :chassisRoll, chassisPitch = :chassisPitch, statusChangeTime = :statusChangeTime, autoAdjustmentStatus = :autoAdjustmentStatus WHERE barrelDirection = :barrelDirection").arg(dbMap[DB_GunMoveInfo]));
    query.bindValue(":elevationAngle", gunMoveData.elevationAngle);
    query.bindValue(":chassisRoll", gunMoveData.chassisRoll);
    query.bindValue(":chassisPitch", gunMoveData.chassisPitch);
    query.bindValue(":statusChangeTime", TimeFormatTrans::TimeFormatTrans::getDateTime(gunMoveData.statusChangeTime));
    query.bindValue(":autoAdjustmentStatus", gunMoveData.autoAdjustmentStatus);
    query.bindValue(":barrelDirection", gunMoveData.barrelDirection);
    return query.exec();
}

bool DbManipulation::updateGunFiringData(const GunFiringData& gunFiringData) {
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET elevationAngle = :elevationAngle, chassisRoll = :chassisRoll, chassisPitch = :chassisPitch, statusChangeTime = :statusChangeTime, firingCompletedSignal = :firingCompletedSignal, recoilStatus = :recoilStatus, muzzleVelocityValid = :muzzleVelocityValid, propellantTemperature = :propellantTemperature, muzzleVelocity = :muzzleVelocity WHERE barrelDirection = :barrelDirection").arg(dbMap[DB_GunShootInfo]));
    query.bindValue(":elevationAngle", gunFiringData.elevationAngle);
    query.bindValue(":chassisRoll", gunFiringData.chassisRoll);
    query.bindValue(":chassisPitch", gunFiringData.chassisPitch);
    query.bindValue(":statusChangeTime", TimeFormatTrans::TimeFormatTrans::getDateTime(gunFiringData.statusChangeTime));
    query.bindValue(":firingCompletedSignal", gunFiringData.firingCompletedSignal);
    query.bindValue(":recoilStatus", gunFiringData.recoilStatus);
    query.bindValue(":muzzleVelocityValid", gunFiringData.muzzleVelocityValid);
    query.bindValue(":propellantTemperature", gunFiringData.propellantTemperature);
    query.bindValue(":muzzleVelocity", gunFiringData.muzzleVelocity);
    query.bindValue(":barrelDirection", gunFiringData.barrelDirection);
    return query.exec();
}

/******************************************************************
 *数据库操作
 *查询数据
*****************************************************************/
QList<DeviceName> DbManipulation::getDeviceNames() {
    QList<DeviceName> deviceList;
    QSqlQuery query(database);
    query.prepare(QString("SELECT device_id, device_name FROM %1").arg(dbMap[DB_Equ_Name]));

    if (query.exec()) {
        while (query.next()) {
            DeviceName device;
            device.deviceAddress = query.value(0).toUInt();  // 获取 device_id
            device.deviceName = query.value(1).toString();   // 获取 device_name
            deviceList.append(device);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }

    return deviceList;
}

QList<DeviceStatusInfo> DbManipulation::getDeviceStatusInfos(const TimeCondition *timeCondition) {
    QList<DeviceStatusInfo> statusList;
    QSqlQuery query(database);
    QString queryString = QString("SELECT device_id, timestamp, status, error_code FROM %1").arg(dbMap[DB_Equ_WorkStat]);

    if (timeCondition && timeCondition->startTime.isValid() && timeCondition->endTime.isValid()) {
        queryString += " WHERE timestamp BETWEEN :startTime AND :endTime";
        query.prepare(queryString);
        query.bindValue(":startTime", timeCondition->startTime);
        query.bindValue(":endTime", timeCondition->endTime);
    } else {
        query.prepare(queryString);
    }

    if (query.exec()) {
        while (query.next()) {
            DeviceStatusInfo statusInfo;
            statusInfo.deviceStatus.deviceAddress = query.value(0).toUInt();  // 获取 device_id

            // 将 QDateTime 转换为 LongDateTime
            QDateTime timestamp = query.value(1).toDateTime();
            statusInfo.dateTime = TimeFormatTrans::getLongDataTime(timestamp);
            statusInfo.deviceStatus.Status = query.value(2).toUInt();  // 获取 status

            // 获取故障信息并转换为 quint8 数组
            QByteArray errorCodeArray = query.value(3).toByteArray();
            memcpy(statusInfo.deviceStatus.faultInfo, errorCodeArray.constData(), sizeof(statusInfo.deviceStatus.faultInfo));

            statusList.append(statusInfo);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }

    return statusList;
}

QList<DeviceErrorInfo> DbManipulation::getDeviceErrorInfos(int statId) {
    QList<DeviceErrorInfo> errorList;
    QSqlQuery query(database);
    query.prepare(QString("SELECT stat_id, error_info FROM %1 WHERE stat_id = :stat_id").arg(dbMap[DB_Equ_ErrorInfo]));
    query.bindValue(":stat_id", statId);

    if (query.exec()) {
        while (query.next()) {
            DeviceErrorInfo errorInfo;
            errorInfo.statId = query.value(0).toInt();         // 获取 stat_id
            errorInfo.errorInfo = query.value(1).toString();   // 获取 error_info
            errorList.append(errorInfo);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }

    return errorList;
}
QList<DeviceTotalWorkTime> DbManipulation::getDeviceTotalWorkTimes(int deviceId) {
    QList<DeviceTotalWorkTime> workTimeList;
    QSqlQuery query(database);
    query.prepare(QString("SELECT device_id, total_worktime FROM %1 WHERE device_id = :device_id").arg(dbMap[DB_Equ_TotalWorkTime]));
    query.bindValue(":device_id", deviceId);

    if (query.exec()) {
        while (query.next()) {
            DeviceTotalWorkTime workTimeInfo;
            workTimeInfo.deviceId = query.value(0).toInt();          // 获取 device_id
            workTimeInfo.totalWorkTime = query.value(1).toInt();     // 获取 total_worktime
            workTimeList.append(workTimeInfo);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }

    return workTimeList;
}

QList<AlarmInfo> DbManipulation::getAlarmInfos(const TimeCondition *timeCondition) {
    QList<AlarmInfo> alarmList;
    QSqlQuery query(database);
    QString queryString = QString("SELECT alarmContent, statusChangeTime, alarmDetails FROM %1").arg(dbMap[DB_AlarmInfo]);

    if (timeCondition && timeCondition->startTime.isValid() && timeCondition->endTime.isValid()) {
        queryString += " WHERE statusChangeTime BETWEEN :startTime AND :endTime";
        query.prepare(queryString);
        query.bindValue(":startTime", timeCondition->startTime);
        query.bindValue(":endTime", timeCondition->endTime);
    } else {
        query.prepare(queryString);
    }

    if (query.exec()) {
        while (query.next()) {
            AlarmInfo alarmInfo;
            alarmInfo.alarmContent = query.value(0).toUInt();  // 获取 alarmContent

            // 将 QDateTime 转换为 LongDateTime
            QDateTime timestamp = query.value(1).toDateTime();
            alarmInfo.statusChangeTime = TimeFormatTrans::getLongDataTime(timestamp);
            alarmInfo.alarmDetails = query.value(2).toUInt();  // 获取 alarmDetails

            alarmList.append(alarmInfo);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }
    return alarmList;
}

QList<GunMoveData> DbManipulation::getGunMoveData(const TimeCondition *timeCondition) {
    QList<GunMoveData> gunMoveList;
    QSqlQuery query(database);
    QString queryString = QString("SELECT barrelDirection, elevationAngle, chassisRoll, chassisPitch, statusChangeTime, autoAdjustmentStatus FROM %1")
            .arg(dbMap[DB_GunMoveInfo]);

    if (timeCondition && timeCondition->startTime.isValid() && timeCondition->endTime.isValid()) {
        queryString += " WHERE statusChangeTime BETWEEN :startTime AND :endTime";
        query.prepare(queryString);
        query.bindValue(":startTime", timeCondition->startTime);
        query.bindValue(":endTime", timeCondition->endTime);
    } else {
        query.prepare(queryString);
    }

    if (query.exec()) {
        while (query.next()) {
            GunMoveData gunMoveData;
            gunMoveData.barrelDirection = query.value(0).toUInt();  // 获取 barrelDirection
            gunMoveData.elevationAngle = query.value(1).toUInt();   // 获取 elevationAngle
            gunMoveData.chassisRoll = query.value(2).toUInt();      // 获取 chassisRoll
            gunMoveData.chassisPitch = query.value(3).toUInt();     // 获取 chassisPitch

            // 将 QDateTime 转换为 LongDateTime
            QDateTime timestamp = query.value(4).toDateTime();
            gunMoveData.statusChangeTime = TimeFormatTrans::getLongDataTime(timestamp);
            gunMoveData.autoAdjustmentStatus = query.value(5).toUInt(); // 获取 autoAdjustmentStatus

            gunMoveList.append(gunMoveData);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }

    return gunMoveList;
}

QList<GunFiringData> DbManipulation::getGunFiringData(const TimeCondition *timeCondition) {
    QList<GunFiringData> gunFiringList;
    QSqlQuery query(database);
    QString queryString = QString("SELECT barrelDirection, elevationAngle, chassisRoll, chassisPitch, statusChangeTime, "
                                  "firingCompletedSignal, recoilStatus, muzzleVelocityValid, propellantTemperature, muzzleVelocity FROM %1")
            .arg(dbMap[DB_GunShootInfo]);

    if (timeCondition && timeCondition->startTime.isValid() && timeCondition->endTime.isValid()) {
        queryString += " WHERE statusChangeTime BETWEEN :startTime AND :endTime";
        query.prepare(queryString);
        query.bindValue(":startTime", timeCondition->startTime);
        query.bindValue(":endTime", timeCondition->endTime);
    } else {
        query.prepare(queryString);
    }

    if (query.exec()) {
        while (query.next()) {
            GunFiringData gunFiringData;
            gunFiringData.barrelDirection = query.value(0).toUInt();  // 获取 barrelDirection
            gunFiringData.elevationAngle = query.value(1).toUInt();   // 获取 elevationAngle
            gunFiringData.chassisRoll = query.value(2).toUInt();      // 获取 chassisRoll
            gunFiringData.chassisPitch = query.value(3).toUInt();     // 获取 chassisPitch

            // 将 QDateTime 转换为 LongDateTime
            QDateTime timestamp = query.value(4).toDateTime();
            gunFiringData.statusChangeTime = TimeFormatTrans::getLongDataTime(timestamp);
            gunFiringData.firingCompletedSignal = query.value(5).toUInt();   // 获取 firingCompletedSignal
            gunFiringData.recoilStatus = query.value(6).toUInt();            // 获取 recoilStatus
            gunFiringData.muzzleVelocityValid = query.value(7).toUInt();     // 获取 muzzleVelocityValid
            gunFiringData.propellantTemperature = query.value(8).toUInt();   // 获取 propellantTemperature
            gunFiringData.muzzleVelocity = query.value(9).toUInt();          // 获取 muzzleVelocity

            gunFiringList.append(gunFiringData);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }

    return gunFiringList;
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



/******************************************************************
 *数据库操作
 *删除数据
*****************************************************************/
//表格内删除数据
bool DbManipulation::deleteData(int index, QString id){
    bool res=false;
    QString delcmd;
    if(openDb()&&isTableExist(dbMap[index]))
    {
        QSqlQuery query(database);

        switch (index) {
        case DB_Equ_Name:
            delcmd = QString("delete from %1 where device_id='%2';").arg(dbMap[index]).arg(id);
            break;
        case DB_Equ_WorkStat:
            delcmd = QString("delete from %1 where stat_id='%2';").arg(dbMap[index]).arg(id);
            break;
        case DB_Equ_ErrorInfo:
            delcmd = QString("delete from %1 where id='%2';").arg(dbMap[index]).arg(id);
            break;
        case DB_Equ_TotalWorkTime:
            delcmd = QString("delete from %1 where id='%2';").arg(dbMap[index]).arg(id);
            break;
        case DB_AlarmInfo:
            delcmd = QString("delete from %1 where id='%2';").arg(dbMap[index]).arg(id);
            break;
        case DB_GunMoveInfo:
            delcmd = QString("delete from %1 where id='%2';").arg(dbMap[index]).arg(id);
            break;
        case DB_GunShootInfo:
            delcmd = QString("delete from %1 where id='%2';").arg(dbMap[index]).arg(id);
            break;
        }
    }
    if(delcmd!="")
        res=query.exec(delcmd);
    if(!res)
        qDebug()<<"dele data failed:"<<query.lastError();
    return res;
}


