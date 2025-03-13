#include "dbmanipulation.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <QCoreApplication>

const QString dataTimeStyle="yyyy-MM-dd HH:mm:ss.zzz";//"yyyy-MM-dd HH:mm:ss.zzz"
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
                            "timestamp TEXT NOT NULL, "
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
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1("
                            "device_id INTEGER PRIMARY KEY, "
                            "total_worktime INTEGER NOT NULL, "
                            "FOREIGN KEY (device_id) REFERENCES %2 (device_id) ON DELETE CASCADE);")
                .arg(dbMap[DB_Equ_TotalWorkTime])
                .arg(dbMap[DB_Equ_Name]);
        break;
    case DB_AlarmInfo:
        createCmd = QString("CREATE TABLE IF NOT EXISTS %1 ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "alarm_content TEXT NOT NULL, "
                            "alarm_status_change_time TEXT, "
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
                            "status_change_time TEXT, "
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
                             "status_change_time TEXT , "
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
            //qDebug()<<"creat table failed:"<<dbMap[index]<<query.lastError();
        }
}

/******************************************************************
 *数据库操作
 *插入数据
*****************************************************************/
bool DbManipulation::insertDeviceName(const DeviceName& deviceName) {
    if(!openDb())
        return false;
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (device_id, device_name) VALUES (:device_id, :device_name)").arg(dbMap[DB_Equ_Name]));
    query.bindValue(":device_id", deviceName.deviceAddress);
    query.bindValue(":device_name", deviceName.deviceName);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to insert :" << query.lastError();
        return false;  // 返回 -1 表示插入失败
    }
}


int DbManipulation::insertDeviceStatusInfo(const DeviceStatusInfo& statusInfo) {
    if(!openDb())
        return false;
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (device_id, timestamp, status, error_code) VALUES (:device_id, :timestamp, :status, :error_code)")
                  .arg(dbMap[DB_Equ_WorkStat]));

    query.bindValue(":device_id", statusInfo.deviceStatus.deviceAddress);

    // 将 LongDateTime 转换为 QDateTime
    QDateTime timestamp(TimeFormatTrans::TimeFormatTrans::getDateTime(statusInfo.dateTime));
    query.bindValue(":timestamp", timestamp.toString(dataTimeStyle));

    query.bindValue(":status", statusInfo.deviceStatus.Status);

    // 将故障信息数组转换为 QByteArray
    query.bindValue(":error_code", QByteArray(reinterpret_cast<const char*>(statusInfo.deviceStatus.faultInfo), sizeof(statusInfo.deviceStatus.faultInfo)));

    // 打印 SQL 查询以调试
    QString boundSql = query.lastQuery();
    QMap<QString, QVariant> boundValues = query.boundValues();
    for (auto it = boundValues.cbegin(); it != boundValues.cend(); ++it) {
        boundSql.replace(it.key(), it.value().toString());
    }
    qDebug() << "Executing SQL:" << boundSql;

    if (query.exec()) {
        // 获取自增的 stat_id 值
        qDebug() << "insert device stat_id:" << query.lastInsertId().toInt();
        return query.lastInsertId().toInt();
    } else {
        qDebug() << "Failed to insert device status info:" << query.lastError();
        return -1;  // 返回 -1 表示插入失败
    }
}


bool DbManipulation::insertDeviceErrorInfo(const DeviceErrorInfo& errorInfo) {
    if(!openDb())
        return false;
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (stat_id, error_info) VALUES (:stat_id, :error_info)").arg(dbMap[DB_Equ_ErrorInfo]));
    query.bindValue(":stat_id", errorInfo.statId);
    query.bindValue(":error_info", errorInfo.errorInfo);
    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to insert :" << query.lastError();
        return false;  // 返回 -1 表示插入失败
    }
}

bool DbManipulation::insertDeviceTotalWorkTime(const DeviceTotalWorkTime& workTimeInfo) {
    if(!openDb())
        return false;
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (device_id, total_worktime) VALUES (:device_id, :total_worktime)")
                  .arg(dbMap[DB_Equ_TotalWorkTime]));

    query.bindValue(":device_id", workTimeInfo.deviceId);
    query.bindValue(":total_worktime", workTimeInfo.totalWorkTime);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to insert :" << query.lastError();
        return false;  // 返回 -1 表示插入失败
    }
}


bool DbManipulation::insertAlarmInfo(const AlarmInfo& alarmInfo) {
    if(!openDb())
        return false;
    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (alarm_content, alarm_status_change_time, alarm_info) VALUES (:alarmContent, :statusChangeTime, :alarmDetails)").arg(dbMap[DB_AlarmInfo]));
    query.bindValue(":alarmContent", alarmInfo.alarmContent);
    query.bindValue(":statusChangeTime",TimeFormatTrans::getDateTime(alarmInfo.statusChangeTime).toString(dataTimeStyle));
    query.bindValue(":alarmDetails", alarmInfo.alarmDetails);
    // 打印 SQL 查询以调试
    QString boundSql = query.lastQuery();
    QMap<QString, QVariant> boundValues = query.boundValues();
    for (auto it = boundValues.cbegin(); it != boundValues.cend(); ++it) {
        boundSql.replace(it.key(), it.value().toString());
    }
    qDebug() << "Executing SQL:" << boundSql;

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to insert:" << query.lastError();
        return false;
    }
}

bool DbManipulation::insertGunMoveData(const GunMoveData& gunMoveData) {
    if(!openDb())
        return false;

    QSqlQuery query(database);
    query.prepare(QString("INSERT INTO %1 (barrel_direction, elevation_angle, chassis_roll_angle, chassis_pitch_angle, status_change_time, auto_move_status) "
                          "VALUES (:barrel_direction, :elevation_angle, :chassis_roll_angle, :chassis_pitch_angle, :status_change_time, :auto_move_status)")
                  .arg(dbMap[DB_GunMoveInfo]));

    query.bindValue(":barrel_direction", gunMoveData.barrelDirection);
    query.bindValue(":elevation_angle", gunMoveData.elevationAngle);
    query.bindValue(":chassis_roll_angle", gunMoveData.chassisRoll);
    query.bindValue(":chassis_pitch_angle", gunMoveData.chassisPitch);
    query.bindValue(":status_change_time", TimeFormatTrans::getDateTime(gunMoveData.statusChangeTime).toString(dataTimeStyle));
    query.bindValue(":auto_move_status", gunMoveData.autoAdjustmentStatus);
    // 打印 SQL 查询以调试
    QString boundSql = query.lastQuery();
    QMap<QString, QVariant> boundValues = query.boundValues();
    for (auto it = boundValues.cbegin(); it != boundValues.cend(); ++it) {
        boundSql.replace(it.key(), it.value().toString());
    }
    qDebug() << "Executing SQL:" << boundSql;

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to insert:" << query.lastError();
        return false;
    }
}


bool DbManipulation::insertGunFiringData(const GunFiringData& gunFiringData) {
    if (!openDb())
        return false;

    QString tableName = dbMap[DB_GunShootInfo];
    QSqlQuery query(database);

    query.prepare(QString("INSERT INTO %1 (barrel_direction, elevation_angle, chassis_roll_angle, chassis_pitch_angle, status_change_time, firing_signal_complete, recoil_status, muzzle_velocity_valid, charge_temperature, muzzle_velocity) "
                          "VALUES (:barrel_direction, :elevation_angle, :chassis_roll_angle, :chassis_pitch_angle, :status_change_time, :firing_signal_complete, :recoil_status, :muzzle_velocity_valid, :charge_temperature, :muzzle_velocity)")
                  .arg(tableName));

    // 绑定所有参数，确保它们与数据库表的字段名称一致
    query.bindValue(":barrel_direction", gunFiringData.attitudeData.barrelDirection);
    query.bindValue(":elevation_angle", gunFiringData.attitudeData.elevationAngle);
    query.bindValue(":chassis_roll_angle", gunFiringData.attitudeData.chassisRoll);
    query.bindValue(":chassis_pitch_angle", gunFiringData.attitudeData.chassisPitch);

    QString formattedTime = TimeFormatTrans::getDateTime(gunFiringData.statusChangeTime).toString(dataTimeStyle);
    query.bindValue(":status_change_time", formattedTime);

    query.bindValue(":firing_signal_complete", gunFiringData.firingCompletedSignal);
    query.bindValue(":recoil_status", gunFiringData.recoilStatus);
    query.bindValue(":muzzle_velocity_valid", gunFiringData.muzzleVelocityValid);
    query.bindValue(":charge_temperature", gunFiringData.propellantTemperature);
    query.bindValue(":muzzle_velocity", gunFiringData.muzzleVelocity);

    // 打印 SQL 查询以调试
    QString boundSql = query.lastQuery();
    QMap<QString, QVariant> boundValues = query.boundValues();
    for (auto it = boundValues.cbegin(); it != boundValues.cend(); ++it) {
        boundSql.replace(it.key(), it.value().toString());
    }
    qDebug() << "Executing SQL:" << boundSql;

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to insert:" << query.lastError().text();
        return false;
    }
}



/******************************************************************
 *数据库操作
 *更新数据
*****************************************************************/
bool DbManipulation::updateDeviceStatusInfo(const DeviceStatusInfo& statusInfo) {
    if(!openDb())
        return false;
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET timestamp = :timestamp, status = :status, error_code = :error_code WHERE device_id = :device_id")
                  .arg(dbMap[DB_Equ_WorkStat]));

    query.bindValue(":device_id", statusInfo.deviceStatus.deviceAddress);

    // 将 LongDateTime 转换为 QDateTime
    query.bindValue(":timestamp", TimeFormatTrans::getDateTime(statusInfo.dateTime).toString(dataTimeStyle));
    query.bindValue(":status", statusInfo.deviceStatus.Status);
    // 将故障信息数组转换为 QByteArray
    query.bindValue(":error_code", QByteArray(reinterpret_cast<const char*>(statusInfo.deviceStatus.faultInfo), sizeof(statusInfo.deviceStatus.faultInfo)));
    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to update :" << query.lastError();
        return false;  // 返回 -1 表示插入失败
    }
}
bool DbManipulation::updateDeviceErrorInfo(const DeviceErrorInfo& errorInfo) {
    if(!openDb())
        return false;
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET error_info = :error_info WHERE stat_id = :stat_id")
                  .arg(dbMap[DB_Equ_ErrorInfo]));

    query.bindValue(":error_info", errorInfo.errorInfo);
    query.bindValue(":stat_id", errorInfo.statId);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to update :" << query.lastError();
        return false;  // 返回 -1 表示插入失败
    }
}

bool DbManipulation::updateDeviceTotalWorkTime(const DeviceTotalWorkTime& workTimeInfo) {
    if(!openDb())
        return false;
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET total_worktime = :total_worktime WHERE device_id = :device_id")
                  .arg(dbMap[DB_Equ_TotalWorkTime]));

    query.bindValue(":total_worktime", workTimeInfo.totalWorkTime);
    query.bindValue(":device_id", workTimeInfo.deviceId);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to update :" << query.lastError();
        return false;  // 返回 -1 表示插入失败
    }
}

bool DbManipulation::updateDeviceTotalWorkTimeAdd1Minute( int deviceId) {
    if(!openDb())
        return false;
    // Step 1: 获取当前的 total_worktime 值
    QSqlQuery query(database);
    query.prepare(QString("SELECT total_worktime FROM %1 WHERE device_id = :device_id").arg(dbMap[DB_Equ_TotalWorkTime]));
    query.bindValue(":device_id", deviceId);

    int currentWorkTime = 0;
    if (query.exec() && query.next()) {
        currentWorkTime = query.value(0).toInt();  // 获取当前 total_worktime
    } else {
        qDebug() << "Failed to retrieve current total work time for device ID:" << deviceId;
        return false;
    }

    // Step 2: 在当前值上加 1
    int updatedWorkTime = currentWorkTime + 1;

    // Step 3: 更新数据库中的 total_worktime 值
    QSqlQuery updateQuery(database);
    updateQuery.prepare(QString("UPDATE %1 SET total_worktime = :total_worktime WHERE device_id = :device_id")
                        .arg(dbMap[DB_Equ_TotalWorkTime]));
    updateQuery.bindValue(":total_worktime", updatedWorkTime);
    updateQuery.bindValue(":device_id", deviceId);

    if (!updateQuery.exec()) {
        qDebug() << "Failed to update total work time for device ID:" << deviceId;
        return false;
    }
    return true;
}
//alarm_content, alarm_status_change_time, alarm_info
bool DbManipulation::updateAlarmInfo(int id,const AlarmInfo& alarmInfo) {
    if(!openDb())
        return false;
    QSqlQuery query(database);
    query.prepare(QString("UPDATE %1 SET alarm_content = :alarmContent alarm_status_change_time = :statusChangeTime, alarm_info = :alarmDetails WHERE id = :id")
                  .arg(dbMap[DB_AlarmInfo]));
    query.bindValue(":alarmContent", alarmInfo.alarmContent);
    query.bindValue(":statusChangeTime", TimeFormatTrans::getDateTime(alarmInfo.statusChangeTime).toString(dataTimeStyle));
    query.bindValue(":alarmDetails", alarmInfo.alarmDetails);
    query.bindValue(":id", id);
    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to update :" << query.lastError();
        return false;  // 返回 -1 表示插入失败
    }
}


bool DbManipulation::updateGunMoveData(int id, const GunMoveData& gunMoveData) {
    if (!openDb())
        return false;

    QString tableName = dbMap[DB_GunMoveInfo];
    QSqlQuery query(database);

    query.prepare(QString("UPDATE %1 SET "
                          "barrel_direction = :barrel_direction, "
                          "elevation_angle = :elevation_angle, "
                          "chassis_roll_angle = :chassis_roll_angle, "
                          "chassis_pitch_angle = :chassis_pitch_angle, "
                          "status_change_time = :status_change_time, "
                          "auto_move_status = :auto_move_status "
                          "WHERE id = :id")
                  .arg(tableName));

    query.bindValue(":barrel_direction", gunMoveData.barrelDirection);
    query.bindValue(":elevation_angle", gunMoveData.elevationAngle);
    query.bindValue(":chassis_roll_angle", gunMoveData.chassisRoll);
    query.bindValue(":chassis_pitch_angle", gunMoveData.chassisPitch);
    query.bindValue(":status_change_time", TimeFormatTrans::getDateTime(gunMoveData.statusChangeTime).toString(dataTimeStyle));
    query.bindValue(":auto_move_status", gunMoveData.autoAdjustmentStatus);
    query.bindValue(":id", id);

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to update:" << query.lastError();
        return false;
    }
}

bool DbManipulation::updateGunFiringData(int id, const GunFiringData& gunFiringData) {
    if (!openDb())
        return false;

    QString tableName = dbMap[DB_GunShootInfo];
    QSqlQuery query(database);

    query.prepare(QString("UPDATE %1 SET "
                          "barrel_direction = :barrel_direction, "
                          "elevation_angle = :elevation_angle, "
                          "chassis_roll_angle = :chassis_roll_angle, "
                          "chassis_pitch_angle = :chassis_pitch_angle, "
                          "status_change_time = :status_change_time, "
                          "firing_signal_complete = :firing_signal_complete, "
                          "recoil_status = :recoil_status, "
                          "muzzle_velocity_valid = :muzzle_velocity_valid, "
                          "charge_temperature = :charge_temperature, "
                          "muzzle_velocity = :muzzle_velocity "
                          "WHERE id = :id")
                  .arg(tableName));

    // 绑定所有参数
    query.bindValue(":barrel_direction", gunFiringData.attitudeData.barrelDirection);
    query.bindValue(":elevation_angle", gunFiringData.attitudeData.elevationAngle);
    query.bindValue(":chassis_roll_angle", gunFiringData.attitudeData.chassisRoll);
    query.bindValue(":chassis_pitch_angle", gunFiringData.attitudeData.chassisPitch);

    QString formattedTime = TimeFormatTrans::getDateTime(gunFiringData.statusChangeTime).toString(dataTimeStyle);
    query.bindValue(":status_change_time", formattedTime);

    query.bindValue(":firing_signal_complete", gunFiringData.firingCompletedSignal);
    query.bindValue(":recoil_status", gunFiringData.recoilStatus);
    query.bindValue(":muzzle_velocity_valid", gunFiringData.muzzleVelocityValid);
    query.bindValue(":charge_temperature", gunFiringData.propellantTemperature);
    query.bindValue(":muzzle_velocity", gunFiringData.muzzleVelocity);

    query.bindValue(":id", id);  // 绑定记录的主键 ID

    // 打印 SQL 查询以调试
    QString boundSql = query.lastQuery();
    QMap<QString, QVariant> boundValues = query.boundValues();
    for (auto it = boundValues.cbegin(); it != boundValues.cend(); ++it) {
        boundSql.replace(it.key(), it.value().toString());
    }
    qDebug() << "Executing SQL:" << boundSql;

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Failed to update:" << query.lastError().text();
        return false;
    }
}

/******************************************************************
 *数据库操作
 *查询数据
*****************************************************************/
QList<DeviceName> DbManipulation::getDeviceNames() {
    openDb();

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
    openDb();
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
    openDb();
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
DeviceTotalWorkTime DbManipulation::getDeviceTotalWorkTimes(int deviceId) {
    openDb();
    DeviceTotalWorkTime workTimeInfo;
    QSqlQuery query(database);
    query.prepare(QString("SELECT device_id, total_worktime FROM %1 WHERE device_id = :device_id").arg(dbMap[DB_Equ_TotalWorkTime]));
    query.bindValue(":device_id", deviceId);

    if (query.exec()) {
        if (query.next()) {

            workTimeInfo.deviceId = query.value(0).toInt();          // 获取 device_id
            workTimeInfo.totalWorkTime = query.value(1).toInt();     // 获取 total_worktime
            //workTimeList.append(workTimeInfo);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }

    return workTimeInfo;
}

QList<AlarmInfo> DbManipulation::getAlarmInfos(const TimeCondition *timeCondition) {
    openDb();
    QList<AlarmInfo> alarmList;
    QSqlQuery query(database);
    QString queryString = QString("SELECT alarm_content, alarm_status_change_time, alarm_info FROM %1").arg(dbMap[DB_AlarmInfo]);

    if (timeCondition && timeCondition->startTime.isValid() && timeCondition->endTime.isValid()) {
        queryString += " WHERE alarm_status_change_time BETWEEN :startTime AND :endTime";
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
    openDb();
    QList<GunMoveData> gunMoveList;
    QSqlQuery query(database);

    QString queryString = QString("SELECT barrel_direction, elevation_angle, chassis_roll_angle, chassis_pitch_angle, status_change_time, auto_move_status FROM %1")
            .arg(dbMap[DB_GunMoveInfo]);

    if (timeCondition && timeCondition->startTime.isValid() && timeCondition->endTime.isValid()) {
        queryString += " WHERE status_change_time BETWEEN :startTime AND :endTime";
        query.prepare(queryString);
        query.bindValue(":startTime", timeCondition->startTime);
        query.bindValue(":endTime", timeCondition->endTime);
    } else {
        query.prepare(queryString);
    }

    if (query.exec()) {
        while (query.next()) {
            GunMoveData gunMoveData;
            gunMoveData.barrelDirection = query.value(0).toFloat();  // 获取 barrel_direction
            gunMoveData.elevationAngle = query.value(1).toFloat();   // 获取 elevation_angle
            gunMoveData.chassisRoll = query.value(2).toFloat();      // 获取 chassis_roll_angle
            gunMoveData.chassisPitch = query.value(3).toFloat();     // 获取 chassis_pitch_angle

            QDateTime timestamp = query.value(4).toDateTime();
            gunMoveData.statusChangeTime = TimeFormatTrans::getLongDataTime(timestamp);

            gunMoveData.autoAdjustmentStatus = query.value(5).toInt(); // 获取 auto_move_status

            gunMoveList.append(gunMoveData);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }

    return gunMoveList;
}


QList<GunFiringData> DbManipulation::getGunFiringData(const TimeCondition *timeCondition) {
    openDb();
    QList<GunFiringData> gunFiringList;
    QSqlQuery query(database);

    // 修改列名以匹配数据库表中的实际列名
    QString queryString = QString("SELECT barrel_direction, elevation_angle, chassis_roll_angle, chassis_pitch_angle, status_change_time, "
                                  "firing_signal_complete, recoil_status, muzzle_velocity_valid, charge_temperature, muzzle_velocity FROM %1")
            .arg(dbMap[DB_GunShootInfo]);

    if (timeCondition && timeCondition->startTime.isValid() && timeCondition->endTime.isValid()) {
        queryString += " WHERE status_change_time BETWEEN :startTime AND :endTime";
        query.prepare(queryString);
        query.bindValue(":startTime", timeCondition->startTime);
        query.bindValue(":endTime", timeCondition->endTime);
    } else {
        query.prepare(queryString);
    }

    if (query.exec()) {
        while (query.next()) {
            GunFiringData gunFiringData;
            gunFiringData.attitudeData.barrelDirection = query.value(0).toUInt();  // 获取 barrel_direction
            gunFiringData.attitudeData.elevationAngle = query.value(1).toUInt();   // 获取 elevation_angle
            gunFiringData.attitudeData.chassisRoll = query.value(2).toUInt();      // 获取 chassis_roll_angle
            gunFiringData.attitudeData.chassisPitch = query.value(3).toUInt();     // 获取 chassis_pitch_angle

            // 将 QDateTime 转换为 LongDateTime
            QDateTime timestamp = query.value(4).toDateTime();
            gunFiringData.statusChangeTime = TimeFormatTrans::getLongDataTime(timestamp);

            gunFiringData.firingCompletedSignal = query.value(5).toUInt();   // 获取 firing_signal_complete
            gunFiringData.recoilStatus = query.value(6).toUInt();            // 获取 recoil_status
            gunFiringData.muzzleVelocityValid = query.value(7).toUInt();     // 获取 muzzle_velocity_valid
            gunFiringData.propellantTemperature = query.value(8).toFloat();  // 获取 charge_temperature
            gunFiringData.muzzleVelocity = query.value(9).toFloat();         // 获取 muzzle_velocity

            gunFiringList.append(gunFiringData);
        }
    } else {
        qDebug() << "Query failed:" << query.lastError();
    }

    return gunFiringList;
}


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


