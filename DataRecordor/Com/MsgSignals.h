#ifndef MSGEVENT_H
#define MSGEVENT_H

#include <QObject>
#include "data.h"

class MsgSignals : public QObject
{
    Q_OBJECT

public:
    // 获取单例实例
    static MsgSignals* getInstance()
    {
        static MsgSignals instance;  // 静态局部变量，程序运行期间只会初始化一次
        return &instance;
    }

    // 删除拷贝构造函数和赋值操作符，防止对象拷贝
    MsgSignals(const MsgSignals&) = delete;
    MsgSignals& operator=(const MsgSignals&) = delete;

signals:
    void canDataSig(CanData);
    void serialDataSig(SerialDataRev);
    void commandDataSig(QByteArray);
private:
    // 构造函数私有化，确保外部无法直接创建对象
    MsgSignals() {}
    ~MsgSignals() {}
};


#endif // MSGEVENT_H
