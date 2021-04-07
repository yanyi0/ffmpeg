#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /*初始化时间*/
    time = new QElapsedTimer();
    timer = new QTimer();
    connect(timer,SIGNAL(timeout()),this,SLOT(slot_timer_timeout()));
}

MainWindow::~MainWindow()
{
    delete ui;
}
QString MainWindow::timeCounterString()
{
    /*最多计算：24天=2073600000ms，也就是576小时*/
    int elapsed = time->elapsed()/1000;
    short int hour = 0;
    short int minute = 0;
    short int second = 0;
    if(elapsed>=3600)
    {
        /*n hour*/
        hour=elapsed/3600;
        elapsed =elapsed%3600;
    }
    else if(elapsed>=60)
    {
        /*n minute*/
        minute=elapsed/60;
        second =elapsed%60;
    }
    else if(elapsed<60)
    {
        /*n second*/
        second =elapsed;
    }
    return QString("%1:%2:%3").arg(hour).arg(minute).arg(second);
}

QStringList MainWindow::timeCounter()
{
    /*最多计算：24天=2073600000ms，也就是576小时*/
    int elapsed = time->elapsed()/1000;
    short int hour = 0;
    short int minute = 0;
    short int second = 0;
    if(elapsed>=3600)
    {
        /*n hour*/
        hour=elapsed/3600;
        elapsed =elapsed%3600;
    }
    else if(elapsed>=60)
    {
        /*n minute*/
        minute=elapsed/60;
        second =elapsed%60;
    }
    else if(elapsed<60)
    {
        /*n second*/
        second =elapsed;
    }
    return QStringList()<<tr("%01").arg(hour)<<tr("%01").arg(minute)<<tr("%01").arg(second);
}

void MainWindow::slot_timer_timeout()
{
    qDebug()<<timeCounter();
    qDebug()<<timeCounterString();
    ui->timerLabel->setText(timeCounterString());
}

void MainWindow::on_startButton_clicked()
{
    qDebug()<<tr("开启计时器");
    time->start();
    timer->start(1000);
}

void MainWindow::on_stopButton_clicked()
{
    qDebug()<<tr("停止计时器");
    timer->stop();
}
