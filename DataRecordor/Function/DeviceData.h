#ifndef DEVICEDATA_H
#define DEVICEDATA_H
#include <QString>
#include <QList>
#include <QMap>
#include <QVariant>


struct CanSignal {
    QString name;       // 信号名称
    int startBit;       // 起始位
    int length;         // 长度
    QString unit;       // 单位
    QString dataType;   // 数据类型
    double factor;      // 比例因子
    double offset;      // 偏移量
    QMap<int, QString> valueTable; // 值表
};

struct CanDataFormat {
    int id;                     // CAN ID
    QString msgName;             // 消息名称
    QList<CanSignal> canSignals;    // 信号列表
};

struct CanDataValue {
    QVariant value      ;
    QString valueDescription;//描述
    QString unit        ;//单位
};



#endif // DEVICEDATA_H
