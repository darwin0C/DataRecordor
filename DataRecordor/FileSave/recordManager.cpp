#include "recordManager.h"
#include <QDebug>
#include "MsgSignals.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QtConcurrent> //
#ifdef LINUX_MODE
#include <sys/statvfs.h>
#endif
RecordManager::RecordManager()
{
    connect(MsgSignals::getInstance(),&MsgSignals::sendCheckDiskSig,this,&RecordManager::onCheckDisk);
    checkTmr = new QTimer(this);
    checkTmr->setInterval(10*1000); // 10 s 足够
    connect(checkTmr, &QTimer::timeout, this, &RecordManager::onCheckDisk);
    checkTmr->start();
    // 定时检查当前文件存在性
    existTmr = new QTimer(this);
    existTmr->setInterval(5*1000);
    connect(existTmr, &QTimer::timeout, this, &RecordManager::onCheckFileExists);
    existTmr->start();
}

QString RecordManager::getRecordData(const SerialDataRev &dataRev)
{
    /* ---------- 1) 拆分日期与时间 ---------- */
    const auto &dt = dataRev.candata.dateTime;

    char dateBuf[12];   // "yyyy_MMdd" => 9 字节 + '\0'
    int  dateLen = std::snprintf(dateBuf, sizeof(dateBuf),
                                 "%04d_%02d%02d",
                                 dt.year, dt.month, dt.day);

    char timeBuf[16];  // "HH:mm:ss.zzz" => 12 字节 + '\0'
    int  timeLen = std::snprintf(timeBuf, sizeof(timeBuf),
                                 "%02d:%02d:%02d.%03d",
                                 dt.hour, dt.minute, dt.second, dt.msec);

    QString date = QString::fromLatin1(dateBuf,  dateLen);
    QString time = QString::fromLatin1(timeBuf,  timeLen);

    /* 先更新日期切换、建新文件等逻辑 */
    checkTime(date, time);
    /* ---------- 2) 继续拼接头部 ---------- */
    char headBuf[64];
    int headLen = std::snprintf(headBuf, sizeof(headBuf),
                                "%s ,prot:%d : ",
                                timeBuf, dataRev.port);
    QString head = QString::fromLatin1(headBuf, headLen);
    // 2) 十六进制 ID 和 data
    QString can_id = QString::number(dataRev.candata.dataid, 16)
            .toUpper()
            .rightJustified(8, '0');
    QByteArray raw(reinterpret_cast<const char*>(dataRev.candata.data), 8);
    QString can_data = QString::fromLatin1(raw.toHex().toUpper());

    QString dateSave = head + "ID:" + can_id + ", data:" + can_data + QLatin1String("\r");

    return dateSave;
}

void RecordManager::checkTime(QString date,QString time)
{

#ifdef TEST_MODE
    if (date == revDate && time.left(5) == revTime)
        return;                         // 早退，减小临界区

#else
    if (date == revDate && time.left(2) == revTime)
        return;                         // 早退，减小临界区
#endif

#ifdef LINUX_MODE
    // --- 新增：计算时间差，避免频繁设置 ---
    QString dateTimeStr = date + " " + time; // 格式: "yyyy_MMdd HH:mm:ss.zzz"
    QDateTime targetTime = QDateTime::fromString(dateTimeStr, "yyyy_MMdd HH:mm:ss.zzz");

    bool needUpdate = true;
    if (targetTime.isValid()) {
        QDateTime currentSysTime = QDateTime::currentDateTime();
        qint64 diff = std::abs(currentSysTime.secsTo(targetTime));

        // 阈值设为 5 秒
        if (diff < 5) {
            needUpdate = false;
        } else {
            qDebug() << "[TimeSync] Drift:" << diff << "s. Updating...";
        }
    }

    SetSysTime(date.left(4) + "-" + date.mid(5,2) + "-" + date.right(2),
               time.left(8));
#endif

    revDate = date;
#ifdef TEST_MODE
    revTime = time.left(5);
#else
    revTime = time.left(2);
#endif

    if (isSDCardOK && revTime.length() >= 2)
        creatNewFile(revDate, revTime);
}

RecordManager::~RecordManager()
{

}


void RecordManager::checkSize(const QString &result)
{
    //qDebug()<<"checkSize"<<result;
    QString dev, use, free, all;
    diskUsed=0;
    diskAll=0;
    diskUsedPercent=0;
    diskFree=0;
    int percent = 0;
    QStringList list = result.split(" ");
    int index = 0;
    for (int i = 0; i < list.count(); i++) {
        QString s = list.at(i).trimmed();
        if (s == "") {
            continue;
        }
        index++;
        if (index == 1) {
            dev = s;
        } else if (index == 2) {
            all = s;
            diskAll=all.toInt();
        } else if (index == 3) {
            use = s;
            diskUsed=use.toInt();
        } else if (index == 4) {
            free = s;
            diskFree=free.toInt();
        } else if (index == 5) {
            percent = s.left(s.length() - 1).toInt();
            diskUsedPercent=percent;
            break;
        }
    }
    if(diskFree<diskMinFree)
    {
        delOldestFile();
    }
}
QString RecordManager::findOldestFile() const
{
    QDirIterator it(gPath, QDir::Files, QDirIterator::Subdirectories);
    QString oldestFile;
    QDateTime oldest = QDateTime::currentDateTimeUtc();
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        if (fi.lastModified() < oldest) {
            oldest = fi.lastModified();
            oldestFile = fi.filePath();
        }
    }
    return oldestFile;
}
void RecordManager::delOldestFile(void)
{
    QString filename=findOldestFile();
    if(filename!="")
    {
        QFile fileTemp(filename);
        QFileInfo fileInfo(filename);
        QString path=fileInfo.absolutePath();

        fileTemp.remove();

        QDir dir(path);
        QFileInfoList filelist=dir.entryInfoList();
        if( filelist.count()==2 && dir.exists())
        {
            dir.removeRecursively();
        }
    }
}
void RecordManager::getAllFileName(QString path, QVector<QString> &path_vec)
{
    QDir dir(path);
    // 列出所有 文件 和 子目录（排除 . 和 ..）
    QFileInfoList infos = dir.entryInfoList(
                QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot
                );
    for (const QFileInfo &info : infos) {
        QString filePath = info.filePath();
        if (info.isFile()) {
            path_vec.push_back(filePath);
        }
        else if (info.isDir()) {
            // 递归子目录
            getAllFileName(filePath, path_vec);
        }
    }
}

void RecordManager::creatNewFile(QString date,QString time)
{
    QString nameToEmit; // 用于存储文件名，以便在锁外发射
    {
        QMutexLocker locker(&fileMutex);
        if (date != currentDate || time != currentTime)
        {
            isTimeSet = false;
            newfileInternal(date, time);
            nameToEmit = gCurrentfileName;
        }
    }
    if (!nameToEmit.isEmpty()) {
        qDebug() << "[RecordManager] Switching file to:" << nameToEmit;
        emit creatFileSig(nameToEmit);
    }
}
void RecordManager::newfileInternal(QString date, QString time)
{
    currentDate = date;
    currentTime = time;
    // 1. 路径与目录逻辑
    QString fileDir = gPath + "ebd_" + date.replace(4, 1, '_');
    QDir dir;
    if (!dir.exists(fileDir)) {
        dir.mkpath(fileDir);
    }
    // 2. 生成文件名
#ifdef TEST_MODE
    QString base = fileDir + "/ebd_can_" + time.left(5).replace(':','_');
#else
    QString base = fileDir + "/ebd_can_" + time.left(2);
#endif
    QFile candidate(base + ".txt");
    // 3. 更新成员变量 (此时处于 caller 的锁保护下)
    gCurrentfileName = candidate.fileName();
    qDebug() << "[RecordManager] Internal created path:" << gCurrentfileName;
}
void RecordManager::newfile(QString date, QString time)
{
    qDebug() << "[RecordManager] newfile Enter. Thread:" << (quint64)QThread::currentThreadId();

    QString nameToEmit; // 用于保存需要发射的文件名
    {
        QMutexLocker locker(&fileMutex); // 获取锁
        newfileInternal(date, time);
        nameToEmit = gCurrentfileName;
    } // --- 临界区结束 ---
    if (!nameToEmit.isEmpty()) {
        qDebug() << "[RecordManager] Emitting signal (Safe): " << nameToEmit;
        emit creatFileSig(nameToEmit);
    }
}

void RecordManager::onCheckDisk()
{
#ifdef LINUX_MODE
    // 1) 打开 /proc/mounts
    QFile mnts("/proc/mounts");
    if (!mnts.open(QIODevice::ReadOnly | QIODevice::Text)) {
        isSDCardOK = false;
        qWarning() << "[RecordManager] error open /proc/mounts,SD card error";
        return;
    }
    QByteArray all = mnts.readAll();
    mnts.close();

    // 2) 找到 mmcblk0p1 对应的挂载点
    QString mountPoint;
    auto lines = all.split('\n');
    for (auto &ln : lines) {
        if (ln.startsWith("/dev/mmcblk")) {
            auto cols = ln.split(' ');
            if (cols.size() >= 2) {
                mountPoint = QString::fromUtf8(cols[1]);
            }
            break;
        }
    }
    if (mountPoint.isEmpty()) {
        isSDCardOK = false;
        qWarning() << "[RecordManager]  /dev/mmcblk0p1 not found";
        return;
    }

    // 3) 调用 statvfs 获取空间
    struct statvfs st;
    if (statvfs(mountPoint.toLocal8Bit().constData(), &st) != 0) {
        isSDCardOK = false;
        qWarning() << "[RecordManager] statvfs 失败:" << strerror(errno);
        return;
    }
    quint64 blockSize = st.f_frsize;
    quint64 totalKB   = (st.f_blocks * blockSize) >> 10;
    quint64 freeKB    = (st.f_bavail * blockSize) >> 10;
    quint64 usedKB    = totalKB - freeKB;
    int pct           = usedKB ? int(usedKB * 100 / totalKB) : 0;

    // 4) 更新成员
    diskAll         = int(totalKB);
    diskFree        = int(freeKB);
    diskUsed        = int(usedKB);
    diskUsedPercent = pct;
    isSDCardOK      = true;
    qDebug()<<"diskAll :"<<diskAll<<" diskFree  :"<<diskFree<<" diskUsedPercent "<<diskUsedPercent;
    // 5) 如剩余空间不足则清理最旧文件
    if (diskFree < diskMinFree) {
        delOldestFile();
    }
#endif
}

void RecordManager::onCheckFileExists()
{
    QString nameToEmit; // 用于保存需要发射的文件名
    {
        QMutexLocker locker(&fileMutex); // 获取锁
        // 检查文件是否存在
        if (!gCurrentfileName.isEmpty() && !QFile::exists(gCurrentfileName)) {
            qDebug() << "[RecordManager] File missing, recreating...";
            newfileInternal(currentDate, currentTime);
            nameToEmit = gCurrentfileName;
        }
    } // --- 临界区结束，解锁 ---
    if (!nameToEmit.isEmpty()) {
        qDebug() << "[RecordManager] Emitting signal (Safe, Recreated): " << nameToEmit;
        emit creatFileSig(nameToEmit);
    }
}
QByteArray RecordManager::HexStringToByteArray(QString HexString)
{
    bool ok;
    QByteArray ret;
    HexString = HexString.remove(QRegExp("\\s"));
    for(int i=0;i<HexString.length()/2;i++)
    {
        char c = HexString.mid(i*2,2).toInt(&ok,16)&0xFF;
        if(ok){
            ret.append(c);
        }
    }
    return ret;
}
void RecordManager::SetSysTime(QString date,QString time)
{
#ifdef LINUX_MODE
    // 使用 QtConcurrent::run 将耗时操作移入后台线程
    // 注意：必须按值传递参数 (QString date, QString time)，确保线程安全
    QtConcurrent::run([date, time](){

        qDebug() << "[TimeSync] Background thread starting set time:" << date << time;
        // 格式化为 "YYYY-MM-DD HH:MM:SS"
        QString cmd = QString("date -s \"%1 %2\"").arg(date).arg(time);
        system(cmd.toLatin1().constData());

        // 同步到硬件时钟 (防止重启失效)
        //system("hwclock -w");

        qDebug() << "[TimeSync] Done.";
    });

    // 标记时间已设置 (主线程变量，注意：这里只是置标志，逻辑上没问题)
    isTimeSet = true;
#endif
}
