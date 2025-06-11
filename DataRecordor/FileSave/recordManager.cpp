#include "recordManager.h"
#include <QDebug>
#include "MsgSignals.h"
#include <QDirIterator>
#include <QFileInfo>
#ifdef LINUX_MODE
#include <sys/statvfs.h>
#endif
RecordManager::RecordManager()
{
    connect(MsgSignals::getInstance(),&MsgSignals::sendCheckDiskSig,this,&RecordManager::onCheckDisk);
    checkTmr = new QTimer(this);
    checkTmr->setInterval(10*1000);        // 10 s 足够
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

    if (date == revDate && time.left(2) == revTime)
        return;                         // 早退，减小临界区

#ifdef LINUX_MODE
    SetSysTime(date.left(4) + "-" + date.mid(5,2) + "-" + date.right(2),
               time.left(8));
#endif

    revDate = date;
    revTime = time.left(2);

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
    //    if(diskFree<diskMinFree-100*1024)
    //    {
    //        process->start("df -k");
    //    }
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
    //    QVector<QString> path_vec;
    //    path_vec.clear();
    //    getAllFileName(gPath,path_vec);
    //    QString filename="";
    //    if(path_vec.count()>0)
    //    {
    //        QFileInfo fileInfo_server(path_vec[0]);
    //        QDateTime tempModifiedTime=fileInfo_server.lastModified().toLocalTime();
    //        filename=path_vec[0];
    //        //QVector<QString>::iterator iter;
    //        for (auto iter=path_vec.begin();iter!=path_vec.end();iter++)
    //        {
    //            QFileInfo fileInfo_server(*iter);
    //            QDateTime lastModifiedTime =fileInfo_server.lastModified().toLocalTime();
    //            if(lastModifiedTime<tempModifiedTime)
    //            {
    //                tempModifiedTime=lastModifiedTime;
    //                filename=*iter;
    //            }
    //        }
    //    }
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
    //qDebug()<<"<<<<<<<<<creatNewFile=========";
    //QFileInfo currntFile(gCurrentfileName);
    if(date!=currentDate || time!=currentTime/*||!currntFile.exists()*/)
    {
        isTimeSet=false;
        currentDate=date;
        currentTime=time;
        newfile(date,time);
    }
}
void RecordManager::newfile(QString date,QString time)
{
    QMutexLocker locker(&fileMutex);    // 保证与其它文件操作互斥
    QString fileDir=gPath+"ebd_"+date.replace(4,1,'_');
    QDir dir;
    if (!dir.exists(fileDir))
    {
        dir.mkpath(fileDir);
    }
    // QString dirPath=fileDir+"/ebd_can_"+time.left(2);
    //qDebug()<<"<<<<<<<<<fileName="<<dirPath;

    //static int index=0;
    QString base = fileDir + "/ebd_can_" + time.left(2);
    QFile candidate(base + ".txt");
    //    while (candidate.exists()) {
    //        candidate.setFileName(base + '_' + QString::number(++index) + ".txt");
    //    }
    gCurrentfileName = candidate.fileName();

    emit creatFileSig(gCurrentfileName);
    //process->start("df -k");
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
    // 定时检查：若文件被删除，则重建同名文件
    if (!gCurrentfileName.isEmpty() && !QFile::exists(gCurrentfileName)) {
        QMutexLocker locker(&fileMutex);
        newfile(currentDate, currentTime);
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
    QString str = "date -s "+date;
    qDebug()<<"time set"<<str;
    system(str.toLatin1().data());
    str = "date -s "+time;
    system(str.toLatin1().data());
    //强制写入到CMOS
    //system("clock -w");
    isTimeSet=true;
#endif
}
