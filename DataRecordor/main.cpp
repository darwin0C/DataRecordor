#include "mainwindow.h"
#include "data.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDebug>

// 日志文件
static QFile logFile;
// 互斥锁，防止多线程竞争
static QMutex mutex;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker locker(&mutex); // 确保多线程安全
    QTextStream out(&logFile);

    // 获取当前时间
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 日志级别
    QString logLevel;
    switch (type) {
    case QtDebugMsg:
        logLevel = "[DEBUG]";
        break;
    case QtWarningMsg:
        logLevel = "[WARNING]";
        break;
    case QtCriticalMsg:
        logLevel = "[CRITICAL]";
        break;
    case QtFatalMsg:
        logLevel = "[FATAL]";
        break;
    default:
        logLevel = "[INFO]";
        break;
    }

    // 写入日志文件
    out << timeStr << " " << logLevel << " " << msg << " (" << context.function << ")" << endl;
    out.flush(); // 确保立即写入

    if (type == QtFatalMsg) {
        abort(); // 终止程序
    }
}

void ensureDirectoryExists(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) {
        if (dir.mkpath(path)) {
            qDebug() << "目录创建成功：" << path;
        } else {
            qDebug() << "目录创建失败：" << path;
        }
    } else {
        qDebug() << "目录已存在：" << path;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef OUTTOFILe
    QString path=gPath+"Log/";
    ensureDirectoryExists(path);
    QString fileName=path+"log_"+QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")+".txt";
    // 设置日志文件路径
    logFile.setFileName(fileName);
    if (!logFile.open(QIODevice::Append | QIODevice::Text)) {
        qDebug() << "无法打开日志文件！";
        // return -1;
    }
    // 安装日志处理函数
    qInstallMessageHandler(customMessageHandler);
#endif
    MainWindow w;
    w.show();
    return a.exec();
}
