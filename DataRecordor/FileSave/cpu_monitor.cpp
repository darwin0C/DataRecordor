// cpu_monitor.cpp
#include "cpu_monitor.h"
#include <QTextStream>
#include <QRegExp>
#include <QtGlobal>

QCpuMonitor::QCpuMonitor(QObject *parent)
    : QObject(parent)
    , statFile("/proc/stat")
    , timer(new QTimer(this))
    , prevIdle(0)
    , prevTotal(0)
{
    // 打开 /proc/stat
    if (!statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("QCpuMonitor: 无法打开 /proc/stat");
    } else {
        // 初始化一次基准值
        readStat(prevIdle, prevTotal);
    }

    connect(timer, &QTimer::timeout, this, &QCpuMonitor::onTimeout);
}

QCpuMonitor::~QCpuMonitor()
{
    statFile.close();
}

void QCpuMonitor::start(int intervalMs)
{
    if (!timer->isActive())
        timer->start(intervalMs);
}

void QCpuMonitor::stop()
{
    if (timer->isActive())
        timer->stop();
}

void QCpuMonitor::onTimeout()
{
    quint64 idle, total;
    if (!readStat(idle, total))
        return;

    quint64 dIdle  = idle  - prevIdle;
    quint64 dTotal = total - prevTotal;
    prevIdle  = idle;
    prevTotal = total;

    double usage = 0.0;
    if (dTotal > 0)
        usage = (1.0 - double(dIdle) / double(dTotal)) * 100.0;
    emit cpuUsage(usage);
}

bool QCpuMonitor::readStat(quint64 &idle, quint64 &total)
{
    if (!statFile.isOpen())
        return false;

    statFile.seek(0);
    QByteArray line = statFile.readLine();
    // 拆分空白
    auto parts = line.split(' ');
    if (parts.size() < 5)
        return false;

    quint64 user   = parts[1].toULongLong();
    quint64 nice   = parts[2].toULongLong();
    quint64 sys    = parts[3].toULongLong();
    quint64 idl    = parts[4].toULongLong();
    quint64 iowait = parts.value(5).toULongLong();
    quint64 irq    = parts.value(6).toULongLong();
    quint64 soft   = parts.value(7).toULongLong();
    quint64 steal  = parts.value(8).toULongLong();

    idle  = idl + iowait;
    total = user + nice + sys + idle + irq + soft + steal;
    return true;
}
