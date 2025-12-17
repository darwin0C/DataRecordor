#ifndef VERSIONUTIL_H
#define VERSIONUTIL_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include <QLocale>

class VersionUtil
{
public:
    // 获取当前编译时间字符串 (格式: yyyy-MM-dd HH:mm:ss)
    static QString getCompileVersion() {
        QString dateStr = QStringLiteral(__DATE__); // e.g., "Jul 18 2025"
        QString timeStr = QStringLiteral(__TIME__); // e.g., "16:23:45"

        // 将英文月份格式转换为 QDateTime
        QLocale locale(QLocale::English);
        QString format = "MMM d yyyy HH:mm:ss";
        // 注意：__DATE__ 中的日期如果是個位數，可能會包含額外的空格，簡化處理合併字符串
        QString rawStr = QString("%1 %2").arg(dateStr).arg(timeStr);

        // 解析时间
        QDateTime dt = locale.toDateTime(rawStr, format);

        // 如果解析失败（防御性编程），返回原始字符串或当前时间
        if (!dt.isValid()) {
            return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        }

        return dt.toString("yyyy-MM-dd HH:mm:ss");
    }

    // 核心管理函数：检查并更新版本文件
    static void checkAndUpdate(const QString &dirPath,QString softVer) {
#ifdef TEST_MODE
        QString currentVer = "TEST_"+softVer+getCompileVersion();
#else
        QString currentVer = softVer+getCompileVersion();
#endif
        QDir dir(dirPath);
        // 如果目录不存在，尝试创建
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        QString filePath = dir.filePath("softversion");
        QFile file(filePath);
        QString savedVer = "";

        // 1. 读取旧版本
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            savedVer = in.readAll().trimmed();
            file.close();
        }

        // 2. 比对
        if (savedVer != currentVer) {
            qDebug() << "[VersionUtil] Version Update Detected!";
            qDebug() << "  Old:" << savedVer;
            qDebug() << "  New:" << currentVer;

            // 3. 写入新版本
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                QTextStream out(&file);
                out << currentVer;
                file.close();
                qDebug() << "[VersionUtil] File updated successfully:" << filePath;
            } else {
                qDebug() << "[VersionUtil] Error writing file:" << filePath;
            }
        } else {
            qDebug() << "[VersionUtil] Version up-to-date:" << currentVer;
        }
    }
};

#endif // VERSIONUTIL_H
