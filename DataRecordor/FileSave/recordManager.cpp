#include "recordManager.h"
#include <QDebug>
#include "MsgSignals.h"
RecordManager::RecordManager()
{
    process = new QProcess(this);
    connect(process, SIGNAL(readyRead()), this, SLOT(readDiskData()));
    connect(MsgSignals::getInstance(),&MsgSignals::sendCheckDiskSig,this,&RecordManager::onCheckDisk);
}

QString RecordManager::getRecordData(SerialDataRev dataRev)
{
    //qDebug() << "getRecordData==========================";
    QString dateSave;

    QString date,time,can_id,can_data;

    int year=dataRev.candata.dateTime.year;

    date = QString("%1_%2%3")
            .arg(year,4,10,QLatin1Char('0'))
            .arg(dataRev.candata.dateTime.month,2,10,QLatin1Char('0'))
            .arg(dataRev.candata.dateTime.day,2,10,QLatin1Char('0'));

    time = QString("%1:%2:%3.%4")
            .arg(dataRev.candata.dateTime.hour,2,10,QLatin1Char('0'))
            .arg(dataRev.candata.dateTime.minute,2,10,QLatin1Char('0'))
            .arg(dataRev.candata.dateTime.second,2,10,QLatin1Char('0'))
            .arg(dataRev.candata.dateTime.msec,3,10,QLatin1Char('0'));

    can_id=QString("%1").arg(dataRev.candata.dataid,8,16,QLatin1Char('0')).toUpper();
    QByteArray array((char *)dataRev.candata.data,8);
    can_data=array.toHex().toUpper();

    dateSave=QString("%1 ,prot:%2 : ID:%3, data:%4\r")
            .arg(time)
            .arg(dataRev.port)
            .arg(can_id)
            .arg(can_data);

    checkTime(date,time);

    return dateSave;
}

void RecordManager::checkTime(QString date,QString time)
{
    if(!isTimeSet)
    {
        QString setData=date.left(4)+"-"+date.mid(5,2)+"-"+date.right(2);
        SetSysTime(setData,time);
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
    if(diskFree<diskMinFree-100*1024)
    {
        process->start("df -k");
    }
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
    QDir *dir=new QDir(path);
    QStringList filter;
    QList<QFileInfo> *fileInfo=new QList<QFileInfo>(dir->entryInfoList(filter));
    for(int i = 0;i<fileInfo->count(); ++i)
    {
        const QFileInfo info_tmp = fileInfo->at(i);
        QString path_tmp = info_tmp.filePath();
        if( info_tmp.fileName()==".." || info_tmp.fileName()=="." )
        {
        }else if(info_tmp.isFile() ){
            path_vec.push_back(path_tmp);
        }else if(info_tmp.isDir()){
            getAllFileName(path_tmp,path_vec);
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
    QFileInfo currntFile(gCurrentfileName);
    if(date!=currentDate || time!=currentTime||!currntFile.exists())
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
    QString finalFileName;
    static int index=0;
    QFile *myFile=new QFile(fileName+".txt");
    QFile *nextFIle=new QFile(fileName+"_1.txt");
    finalFileName=fileName+".txt";

    while ((*myFile).exists()||(*nextFIle).exists()) {
        index++;
        myFile=new QFile(fileName+"_"+QString::number(index,10)+".txt");
        nextFIle=new QFile(fileName+"_"+QString::number(index+1,10)+".txt");
        finalFileName=fileName+"_"+QString::number(index,10)+".txt";
    }
    index=0;
    delete myFile;
    delete nextFIle;

    gCurrentfileName=finalFileName;

    emit creatFileSig(finalFileName);
    process->start("df -k");
}
void RecordManager::onCheckDisk()
{
    //qDebug()<<"onCheckDisk";
#ifdef LINUX_MODE
    process->start("df -k");
#endif

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
