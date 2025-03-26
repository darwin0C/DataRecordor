#include "tcpclient.h"


#include <QFile>
#include <QDataStream>
#include <QDebug>

TcpClient::TcpClient( const QString &host, int port, QObject *parent)
    : QThread(parent), host(host), port(port)
{

}

void TcpClient::run()
{
    sendData();
}

void TcpClient::transPic(const QString &fileName)
{
    filePath=fileName;
    this->start();
}

void TcpClient::sendData() {

    QTcpSocket tcpSocket;
    mutex.tryLock(3000);
    tcpSocket.connectToHost(host, port);
    if (tcpSocket.waitForConnected()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            int totalSize = data.size();
            int bytesSent = 0;
            int index=0;
            while (bytesSent < totalSize) {
                int bytesLeft = totalSize - bytesSent;
                int chunkSize = qMin(bytesLeft, 1024);
                QByteArray chunk = data.mid(bytesSent, chunkSize);

                // 发送当前包的大小信息，确保接收方知道应该读取多少数据
                //QByteArray header;
                //QDataStream headerStream(&header, QIODevice::WriteOnly);
                //headerStream << static_cast<quint32>(chunkSize);
                //tcpSocket.write(header); // 发送头部
                tcpSocket.write(chunk); // 发送数据包
                tcpSocket.waitForBytesWritten();
                bytesSent += chunkSize;
                msleep(5);
                qDebug()<<"packet:"<<index++;
            }
            // tcpSocket.write(data);
            tcpSocket.waitForBytesWritten();
            tcpSocket.disconnectFromHost();
        }
    }
    mutex.unlock();
    // 将数据分割成多个包，每个包最大1024字节
}


