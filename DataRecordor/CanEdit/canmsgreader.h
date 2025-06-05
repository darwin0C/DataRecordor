#ifndef CANMSGREADER_H
#define CANMSGREADER_H

#include <QObject>
#include "DeviceData.h"
#include "data.h"

class CanMsgReader : public QObject
{
    Q_OBJECT
    QMap<QString,CanDataValue> parseCanData(const QByteArray &canFrame, const QList<CanDataFormat> &canDataList, int canId);
    quint64 extractBits(const QByteArray &data, int startBit, int length);
    QList<CanDataFormat> canDataList;
public:
    explicit CanMsgReader(QObject *parent = nullptr);
    // 获取单例实例
    static CanMsgReader* Instance()
    {
        static CanMsgReader instance;  // 静态局部变量，程序运行期间只会初始化一次
        return &instance;
    }
    // 删除拷贝构造函数和赋值操作符，防止对象拷贝
    CanMsgReader(const CanMsgReader&) = delete;
    CanMsgReader& operator=(const CanMsgReader&) = delete;

    bool readCanDataFromXml(const QString &fileName);
    QMap<QString,CanDataValue> getValues(const CanData &data);
    QList<CanDataFormat> getCanDataList();
signals:

};

#endif // CANMSGREADER_H
