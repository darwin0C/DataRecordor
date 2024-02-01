#include "recordthread.h"

RecordThread::RecordThread()
{
    process = new QProcess(this);
    connect(process, SIGNAL(readyRead()), this, SLOT(readDiskData()));
}



void RecordThread::timerProcessSerialData(void)
{
    int len=1024;
    static int writeTimes=0;
    if(txtOutput!=nullptr && isSDCardOK)
    {
        writeTimes++;
        //qDebug()<<dataRevCount;
        while(dataToWrite.length()>len)
        {
            writeTimes=0;
            try
            {
                mutex.lock();
                QString d=dataToWrite.left(len);
                dataToWrite.remove(0,len);
                *txtOutput << d;
                txtOutput->flush();
                mutex.unlock();
            }
            catch (std::exception &e)
            {
            }
        }
        if(dataToWrite.length()>0)
        {
            mutex.lock();
            *txtOutput << dataToWrite;
            dataToWrite="";
            mutex.unlock();
        }
        if(writeTimes>1000)
        {
            writeTimes=0;
            txtOutput->flush();
        }
    }
}

void RecordThread::WriteToFile()
{

    while(canDataRev.length()>=26*2)
    {
        if((canDataRev.left(2).toInt(NULL,16)==0xC1)&& (canDataRev.mid(2,2).toInt(NULL,16)==0x0A))
        {
            QString data=canDataRev.left(26*2);
            mutex.lock();
            if(checkData(data))
            {
                canDataRev.remove(0,26*2);
                QString saveDate=getRecordData(data);
                if(saveDate!="")
                {
                    dataToWrite+=saveDate+"\n";
                }
            }
            else
            {
                dataToWrite+=canDataRev.left(4);
                canDataRev.remove(0,4);
                int index=canDataRev.indexOf("C10A");
                if(index>0)
                {
                    dataToWrite+=canDataRev.left(index)+"\n";
                    canDataRev.remove(0,index);
                }
                else
                {
                    dataToWrite+="\n";
                }
            }
            mutex.unlock();
        }
        else if(canDataRev.length()>=4)
        {
            QString errorData="";
            mutex.lock();
            if(canDataRev.indexOf("C10A")>0)
            {
                errorData=canDataRev.mid(0,canDataRev.indexOf("C10A"));
                canDataRev.remove(0,canDataRev.indexOf("C10A"));
            }
            else
            {
                errorData=canDataRev;
                canDataRev="";
            }
            //errorDataCounts++;
            dataToWrite+=errorData+"\n";
            mutex.unlock();
        }
    }
}
QString RecordThread::getRecordData(QString dataRev)
{
    QString dateSave=dataRev;
    if(checkData(dataRev))
    {
        QString date,time,can_id,can_data;

        QString port=dataRev.mid(2*2,2);

        int type=dataRev.mid(2*2,2).toInt(NULL,16);
        int year=dataRev.mid(4*2,2).toInt(NULL,16)*256+dataRev.mid(3*2,2).toInt(NULL,16);
        date = QString("%1_%2%3")
                .arg(year,4,10,QLatin1Char('0'))
                .arg(dataRev.mid(5*2,2).toInt(NULL,16),2,10,QLatin1Char('0'))
                .arg(dataRev.mid(6*2,2).toInt(NULL,16),2,10,QLatin1Char('0'));
        time = QString("%1:%2:%3.%4")
                .arg(dataRev.mid(7*2,2).toInt(NULL,16),2,10,QLatin1Char('0'))
                .arg(dataRev.mid(8*2,2).toInt(NULL,16),2,10,QLatin1Char('0'))
                .arg(dataRev.mid(9*2,2).toInt(NULL,16),2,10,QLatin1Char('0'))
                .arg((dataRev.mid(11*2,2)+dataRev.mid(10*2,2)).toInt(NULL,16),3,10,QLatin1Char('0'));

        can_id=dataRev.mid(16*2,2)+dataRev.mid(15*2,2)+dataRev.mid(14*2,2)+dataRev.mid(13*2,2);
        can_data=dataRev.mid(17*2,16);
        //dateSave=time+":ID:"+can_id+", data:"+can_data;
        dateSave=time+",prot:"+port+":ID:"+can_id+", data:"+can_data;
        if(!isTimeSet)
        {
            QString setData=date.left(4)+"-"+date.mid(5,2)+"-"+date.right(2);
            SetSysTime(setData,time);
        }
        if(type==0x0B)//指令数据
        {
            if(can_data=="EF0000FFFFFCFFFF")//删除所有数据
            {
                delAllFiles();
            }
        }
        if(date!=revDate || time.left(2)!=revTime)
        {
            revDate=date;
            revTime=time.left(2);
            process->start("df -k");
        }
        fileMutex.lock();
        if(isSDCardOK)
        {
            if(!isCreatingFile&&revTime.length()>=2)
                creatNewFile(revDate,revTime);
        }
        fileMutex.unlock();
    }
    else {
        //errorDataCounts++;
    }
    return dateSave;
}
bool RecordThread::checkData(QString dataRev)//校验数据
{
    int len=dataRev.length();
    if((dataRev.left(2).toInt(NULL,16))==0xC1 && len==26*2)
    {
        if(getCheckNum(dataRev)==dataRev.right(2).toInt(NULL,16))
            return true;
    }
    return false;
}
int RecordThread::getCheckNum(QString data)
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
void RecordThread::readDiskData()
{
    isSDCardOK=false;
    while (!process->atEnd()) {
        QString result = QLatin1String(process->readLine());
        if (result.startsWith("/dev/mmcblk")) {
            checkSize(result);
            isSDCardOK=true;
            break;
        }
    }
}
void RecordThread::checkSize(const QString &result)
{
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

void RecordThread::delOldestFile(void)
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
        QVector<QString>::iterator iter;
        for (iter=path_vec.begin();iter!=path_vec.end();iter++)
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
void RecordThread::getAllFileName(QString path, QVector<QString> &path_vec)
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
void RecordThread::delAllFiles(void)
{
    if(file!=nullptr)
    {
        file->close();
        file=nullptr;
    }

    QString del_file = gPath;
    QDir dir;
    if (dir.exists(del_file))
    {
        dir.setPath(del_file);
        dir.removeRecursively();
    }
}
void RecordThread::creatNewFile(QString date,QString time)
{
    QFileInfo currntFile(gCurrentfileName);
    if(date!=currentDate || time!=currentTime||file==nullptr||!currntFile.exists())
    {
        isCreatingFile=true;
        currentDate=date;
        currentTime=time;
        newfile(date,time);
    }
}
void RecordThread::newfile(QString date,QString time)
{
    //mutex.lock();
    if(!isSDCardOK)
    {
        return;
    }
    if(file!=nullptr)
    {
        try {
            if(txtOutput!=nullptr)
                txtOutput->flush();
            file->close();
        } catch (std::exception &e) {

        }
    }
    QString fileDir=gPath+"ebd_"+date.replace(4,1,'_');
    QDir dir;
    if (!dir.exists(fileDir))
    {
        dir.mkpath(fileDir);
    }
    QString fileName=fileDir+"/ebd_can_"+time.left(2);
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
    file=new QFile(finalFileName);

    if((*file).open(QIODevice::ReadWrite| QIODevice::Text | QIODevice::Append))
    {
        txtOutput =new QTextStream(file);
        //process->start("df -k");
    }
}
QByteArray RecordThread::HexStringToByteArray(QString HexString)
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
void RecordThread::SetSysTime(QString date,QString time)
{
    QString str = "date -s "+date;
    system(str.toLatin1().data());
    str = "date -s "+time;
    system(str.toLatin1().data());
    //强制写入到CMOS
    //system("clock -w");
    isTimeSet=true;
}
