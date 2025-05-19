#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include <QObject>
#include <QFile>
#include <QTimer>

class QCpuMonitor : public QObject {
    Q_OBJECT
public:
    explicit QCpuMonitor(QObject *parent = nullptr);
    ~QCpuMonitor() override;

    /// 启动高频采样，intervalMs：采样间隔（毫秒）
    void start(int intervalMs);
    /// 停止采样
    void stop();

signals:
    /// 发射当前 CPU 使用率，取值范围 [0.0, 100.0]
    void cpuUsage(double percent);

private slots:
    void onTimeout();

private:
    QFile    statFile;    // /proc/stat
    QTimer  *timer;       // 采样定时器
    quint64  prevIdle;    // 上次空闲时间
    quint64  prevTotal;   // 上次总时间

    /// 从 /proc/stat 读出 idle 和 total 时钟计数
    bool readStat(quint64 &idle, quint64 &total);
};

#endif // CPU_MONITOR_H
