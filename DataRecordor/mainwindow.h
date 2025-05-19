#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "FileSave/qfilesavethead.h"
#include <QTimer>
#include <QDebug>
#include "data.h"
#include "commanager.h"
#include "CommandCtrol.h"
#include <QTcpServer>
#include <QTcpSocket>
#include "cpu_monitor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QFileSaveThead *mySaveDataThread=nullptr;
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_2_clicked();

    void timerUpdate();
    void timerSendStatus();
    void ServerNewConnection();
    void socket_Read_Data();
    void socket_Disconnected();
    void changeLEDStat();
private:
    Ui::MainWindow *ui;
    ComManager *com=nullptr;
    QTimer* ledTimer;
    QThread* ledTimerThread;
    QTimer *timerStatus;
    QThread* StatusTimerThread;
    QThread* cpuThread;
    CommandCtrol *commandCtrol;
    QThread* commandThread;
    int ledBlankTimes = 0;
    QTcpServer* mp_TCPServer;
    QTcpSocket* mp_TCPSocket;
    QCpuMonitor  *monitor=nullptr;
    void socket_Send_Data(QString dataSend);

    void startRecord();
    void blankLED();
    void sendData(QByteArray dataArray);

    void startLEDThread();
    void startStatus();
    void startCommandCtrl();
    void sendtestData();
    void Test();
    void startcpuMonitor();
signals:
    void sendCanData(CanDataBody);
    void delAllFilesSig();
};
#endif // MAINWINDOW_H
