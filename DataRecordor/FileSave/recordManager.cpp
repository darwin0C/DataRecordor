#include "recordManager.h"
#include <QDebug>
#include "MsgSignals.h"
#include <QFileInfo>
#include <sys/statvfs.h>

RecordManager::RecordManager()
{
    process = new QProcess(this);
    connect(process, SIGNAL(readyRead()), this, SLOT(readDiskData()));
    connect(MsgSignals::getInstance(),&MsgSignals::sendCheckDiskSig,this,&RecordManager::onCheckDisk);
    // 定时检查当前文件存在性
    existTmr = new QTimer(this);
    existTmr->setInterval(5000);
    connect(existTmr, &QTimer::timeout, this, &RecordManager::onCheckFileExists);
    existTmr->start();
}

QString RecordManager::getRecordData(SerialDataRev dataRev)
{
    //qDebug() << "getRecordData==========================";

    // 1) 用 QDateTime 格式化，替代多次 QString::arg
    QDateTime dt(
                QDate(dataRev.candata.dateTime.year,
                      dataRev.candata.dateTime.month,
                      dataRev.candata.dateTime.day),
                QTime(dataRev.candata.dateTime.hour,
                      dataRev.candata.dateTime.minute,
                      dataRev.candata.dateTime.second,
                      dataRev.candata.dateTime.msec)
                );
    QString date = dt.date().toString("yyyy_MMdd");           // e.g. "2025_0404"
    QString time = dt.time().toString("HH:mm:ss.zzz");        // e.g. "12:34:56.789"

    // 2) 十六进制 ID 和 data
    QString can_id = QString::number(dataRev.candata.dataid, 16)
            .toUpper()
            .rightJustified(8, '0');
    QByteArray raw(reinterpret_cast<const char*>(dataRev.candata.data), 8);
    QString can_data = QString::fromLatin1(raw.toHex().toUpper());

    QString dateSave = time + " ,prot:" + QString::number(dataRev.port) +
            " : ID:" + can_id + ", data:" + can_data + "\r";

    checkTime(date,time);

    return dateSave;
}

void RecordManager::checkTime(QString date,QString time)
{
    if(!isTimeSet)
    {
        QString setData=date.left(4)+"-"+date.mid(5,2)+"-"+date.right(2);
        SetSysTime(setData,time.left(8));
    }
    if(date!=revDate || time.left(2)!=revTime)
    {
        revDate=date;
        revTime=time.left(2);
    }
    fileMutex.lock();
    //qDebug()<<"<<<<<<<<<if(isSDCardOK)=========";
    if(isSDCardOK)
    {
        if(revTime.length()>=2)
            creatNewFile(revDate,revTime);
    }
    fileMutex.unlock();
}
RecordManager::~RecordManager()
{
    process->kill();
}

bool RecordManager::checkData(QString dataRev)//校验数据
{
    int len=dataRev.length();
    if((dataRev.left(2).toInt(NULL,16))==0xC1 && len==26*2)
    {
        if(getCheckNum(dataRev)==dataRev.right(2).toInt(NULL,16))
            return true;
    }
    return false;
}
int RecordManager::getCheckNum(QString data)
{
    int sum=0;
    if(data.length()>4)
    {
        for(int i =1;i<data.length()/2-1;i++)
        {
            sum+=data.mid(i*2,2).toInt(NULL,16);
        }
        sum=sum&0xFF;
    }
    return sum;
}
void RecordManager::readDiskData()
{
    isSDCardOK=true;
    while (!process->atEnd()) {
        QString result = QLatin1String(process->readLine());
        if (result.startsWith("/dev/mmcblk")) {
            checkSize(result);
            isSDCardOK=true;
            break;
        }
        isSDCardOK=false;
    }
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

void RecordManager::delOldestFile(void)
{
    QVector<QString> path_vec;
    path_vec.clear();
    getAllFileName(gPath,path_vec);
    QString filename="";
    if(path_vec.count()>0)
    {
        QFileInfo fileInfo_server(path_vec[0]);
        QDateTime tempModifiedTime=fileInfo_server.lastModified().toLocalTime();
        filename=path_vec[0];
        //QVector<QString>::iterator iter;
        for (auto iter=path_vec.begin();iter!=path_vec.end();iter++)
        {
            QFileInfo fileInfo_server(*iter);
            QDateTime lastModifiedTime =fileInfo_server.lastModified().toLocalTime();
            if(lastModifiedTime<tempModifiedTime)
            {
                tempModifiedTime=lastModifiedTime;
                filename=*iter;
            }
        }
    }
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
void RecordManager::delAllFiles(void)
{
    QString del_file = gPath;
    QDir dir;
    if (dir.exists(del_file))
    {
        dir.setPath(del_file);
        dir.removeRecursively();
    }
}
void RecordManager::creatNewFile(QString date,QString time)
{
    //qDebug()<<"<<<<<<<<<creatNewFile=========";
    //QFileInfo currntFile(gCurrentfileName);
    if(date!=currentDate || time!=currentTime/*||!currntFile.exists()*/)
    {
        currentDate=date;
        currentTime=time;
        newfile(date,time);
    }
}
void RecordManager::newfile(QString date,QString time)
{
    QString fileDir=gPath+"ebd_"+date.replace(4,1,'_');
    QDir dir;
    if (!dir.exists(fileDir))
    {
        dir.mkpath(fileDir);
    }
    QString fileName=fileDir+"/ebd_can_"+time.left(2);
    qDebug()<<"<<<<<<<<<fileName="<<fileName;

    static int index=0;
    QFile myFile(fileName + ".txt");
    QFile nextFile(fileName + "_1.txt");
    while (myFile.exists() || nextFile.exists()) {
        ++index;
        myFile.setFileName(fileName + "_" + QString::number(index) + ".txt");
        nextFile.setFileName(fileName + "_" + QString::number(index+1) + ".txt");
    }
    gCurrentfileName = myFile.fileName();
    emit creatFileSig(gCurrentfileName);
    //process->start("df -k");
}
//void RecordManager::onCheckDisk()
//{
//    //qDebug()<<"onCheckDisk";
//#ifdef LINUX_MODE
//    process->start("df -k");
//#endif

//}
void RecordManager::onCheckDisk()
{
    // 1) 打开 /proc/mounts
    QFile mnts("/proc/mounts");
    if (!mnts.open(QIODevice::ReadOnly | QIODevice::Text)) {
        isSDCardOK = false;
        qWarning() << "[RecordManager] 无法打开 /proc/mounts";
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
    //qDebug()<<"diskAll :"<<diskAll<<" diskFree  :"<<diskFree<<" diskUsedPercent "<<diskUsedPercent;
    // 5) 如剩余空间不足则清理最旧文件
    if (diskFree < diskMinFree) {
        delOldestFile();
    }
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
    system(str.toLatin1().data());
    str = "date -s "+time;
    system(str.toLatin1().data());
    //强制写入到CMOS
    //system("clock -w");
    isTimeSet=true;
#endif
}
