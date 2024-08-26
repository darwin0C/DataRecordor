#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "FileSave/qfilesavethead.h"
#include <QTimer>
#include <QDebug>
#include "data.h"
#include "commanager.h"
#include "CommandCtrol.h"

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
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void timerUpdate();
    void timerSendStatus();
private:
    Ui::MainWindow *ui;
    ComManager *com=nullptr;
    QTimer* ledTimer;
    QThread* ledTimerThread;
    QTimer *timerStatus;
    QThread* StatusTimerThread;
    CommandCtrol *commandCtrol;
    QThread* commandThread;
    //void sendData();

    void startRecord();
    void blankLED();
    void sendData(QByteArray dataArray);

    void startLEDThread();
    void startStatus();
    void startCommandCtrl();
signals:
    void sendCanData(CanDataBody);
};
#endif // MAINWINDOW_H
