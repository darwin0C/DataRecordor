#include "mainwindow.h"
#include "data.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QThread>
#include <QDir>

// --- Linux 系统级头文件 (用于崩溃捕获) ---
#ifdef LINUX_MODE
#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#endif
// 日志文件对象
static QFile logFile;
// 互斥锁，防止多线程写日志时冲突
static QMutex logMutex;

// ==========================================
// 1. 崩溃信号处理函数 (Crash Handler)
// ==========================================
#ifdef LINUX_MODE
void signalHandler(int signum) {
    // 防止由信号处理函数引发的递归崩溃
    signal(signum, SIG_DFL);

    const int len = 32;
    void *buffer[len];
    // 获取当前堆栈的层级深度和地址
    int nptrs = backtrace(buffer, len);

    fprintf(stderr, "\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    fprintf(stderr, "!!! CRITICAL ERROR CAUGHT: Signal %d !!!\n", signum);
    fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    // 打印堆栈地址 (嵌入式上通常没有符号，需要用 addr2line 反查)
    fprintf(stderr, "Backtrace Addresses (Use addr2line to resolve):\n");
    for (int i = 0; i < nptrs; i++) {
        fprintf(stderr, "[%02d]: %p\n", i, buffer[i]);
    }
    fprintf(stderr, "----------------------------------------\n");

    // 尝试将崩溃信息也写入日志文件(如果不加锁可能会死锁，但在崩溃前最后一搏值得尝试)
    if (logFile.isOpen()) {
        QString crashMsg = QString("CRASH SIGNAL RECEIVED: %1").arg(signum);
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
            << " [FATAL] " << crashMsg << endl;
        logFile.flush();
    }

    // 再次触发信号，让系统生成 core dump (如果开启了 ulimit -c)
    raise(signum);
}
#endif

// ==========================================
// 2. 自定义 Qt 消息处理函数 (Message Handler)
// ==========================================
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 加锁，防止多线程日志错乱
    QMutexLocker locker(&logMutex);

    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 获取当前线程 ID (关键调试信息)
    quint64 threadId = (quint64)QThread::currentThreadId();

    QString logLevel;
    switch (type) {
    case QtDebugMsg:    logLevel = "[DEBUG]"; break;
    case QtWarningMsg:  logLevel = "[WARN ]"; break; // 对齐美观
    case QtCriticalMsg: logLevel = "[CRIT ]"; break;
    case QtFatalMsg:    logLevel = "[FATAL]"; break;
    default:            logLevel = "[INFO ]"; break;
    }

    // 格式化日志内容: 时间 [线程ID] [级别] 内容 (文件:行号)
    // 注意：Release 模式下 context.file 可能为空，取决于编译选项
    QString fileInfo = "";
    if (context.file) {
        // 只保留文件名，去掉冗长的路径
        QString shortFileName = QString(context.file);
        int lastSlash = shortFileName.lastIndexOf('/');
        if (lastSlash != -1) shortFileName = shortFileName.mid(lastSlash + 1);
        fileInfo = QString(" (%1:%2)").arg(shortFileName).arg(context.line);
    }

    QString formattedMsg = QString("%1 [T:%2] %3 %4%5")
            .arg(timeStr)
            .arg(threadId, 0, 16) // 16进制显示线程ID
            .arg(logLevel)
            .arg(msg)
            .arg(fileInfo);

    // 1. 输出到标准控制台 (方便调试串口/SSH 查看)
    fprintf(stdout, "%s\n", formattedMsg.toLocal8Bit().constData());
    fflush(stdout);

    // 2. 写入日志文件
    if (logFile.isOpen()) {
        QTextStream out(&logFile);
        out << formattedMsg << endl; // endl 会自动 flush，但在嵌入式上如果写入太频繁可能会卡
        // out.flush(); // endl 已经包含了 flush
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

void ensureDirectoryExists(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) {
        if (dir.mkpath(path)) {
            qDebug() << "Log dir created:" << path;
        } else {
            qWarning() << "Failed to create log dir:" << path;
        }
    }
}

int main(int argc, char *argv[])
{
#ifdef LINUX_MODE
    // --- 注册 Linux 信号处理 (在 QApplication 之前) ---
    signal(SIGSEGV, signalHandler); // 捕获段错误 (内存非法访问)
    signal(SIGFPE,  signalHandler); // 捕获浮点错误 (除以零等)
    signal(SIGABRT, signalHandler); // 捕获 Abort 信号
#endif
    QApplication a(argc, argv);


    // 建议：确保 gPath 是绝对路径且在可写分区 (如 /mnt/sdcard/ 或 /tmp/)
    // 假设 data.h 中定义了 gPath
    QString logPath = gPath + "Log/";
    ensureDirectoryExists(logPath);

    QString fileName = logPath + "log_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt";

    logFile.setFileName(fileName);
    // 使用 Unbuffered 模式可能更安全，但会降低性能。Text 模式足矣。
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << fileName << "Logging to console only.";
    } else {
        qDebug() << "Log file opened:" << fileName;
        // 安装 Qt 消息拦截器
        qInstallMessageHandler(customMessageHandler);
    }


    MainWindow w;
    w.show();

    return a.exec();
}
