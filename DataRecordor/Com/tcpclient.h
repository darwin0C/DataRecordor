#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>

#include <QThread>
#include <QTcpSocket>
#include <QMutex>
#include <QTimer>

class TcpClient : public QThread {
    Q_OBJECT
public:
    TcpClient( const QString &host, int port, QObject *parent = nullptr);
    void run() override;

    void transPic(const QString &fileName);
private:
    QString filePath;
    QString host;
    int port;
    QMutex mutex;
    void sendData();
    bool isConnected=false;

};



#endif // TCPCLIENT_H
