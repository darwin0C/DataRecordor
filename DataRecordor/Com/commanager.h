#ifndef COMMANAGER_H
#define COMMANAGER_H

#include <QObject>
#include "qmycom.h"
#include "qmynetcom.h"
#include <QTimer>
#include "tcpclient.h"
#ifndef NET_COM

#endif
class ComManager : public QObject
{
    Q_OBJECT
    QMyCom *serialCom1=nullptr;
    QMyCom *serialCom2=nullptr;
    QMyCom *serialCom3=nullptr;

    int mySelfSocketPort;  //自己的端口号
    QString      mySelfSocketIP;    //自己的IP

    unsigned int  myComBdCastSocketPort;

    QMyNetCom *myNetComInterface;
    QMyNetCom *myNetBdCastInterface;
    QMyNetCom *myGroupCastInterface;
    int sendNetData(QByteArray array, QString ip, int port);
    int netSocketPort;//
    QString netSocketIP;//
    TcpClient *photoTransObj=NULL;
    QMyCom *startSerial(QString portNum);
public:
    explicit ComManager(QObject *parent = nullptr);
    ~ComManager();
    int sendCanData(uint canID, uchar *buff, unsigned char len);

    // 获取单例实例
    static ComManager* instance()
    {
        static ComManager instance;  // 静态局部变量，程序运行期间只会初始化一次
        return &instance;
    }
    // 删除拷贝构造函数和赋值操作符，防止对象拷贝
    ComManager(const ComManager&) = delete;
    ComManager& operator=(const ComManager&) = delete;

    int sendSerialData(QByteArray array);
    int sendData2Command(unsigned char cmdcode, unsigned char *data, int len);
    int sendData2Command(unsigned char reciveCode, unsigned char cmdcode1, unsigned char cmdcode2, unsigned char *data, int len, QString ip, int port);
    int sendNetDataDirectly(QByteArray array, QString ip, int port);
    void senSerialDataByCom(QByteArray array, int comIndex);

signals:
    void sendCanMegSig(QByteArray);
    void jsContrlSig();
};

#endif // COMMANAGER_H
