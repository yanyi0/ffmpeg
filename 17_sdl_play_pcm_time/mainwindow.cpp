#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <SDL2/SDL.h>


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

void showVersion(){
    SDL_version version;
    SDL_VERSION(&version);
    qDebug() << version.major << version.minor << version.patch;
}
//获取音频总时长
void MainWindow::changeButtonText(QString str){
    qDebug() << "changeButtonText";
    qDebug() << str;
    ui->timeAllButton->setText(str);
}
//开始播放音频，启动计时器
void MainWindow::timerStartText(){
//    qDebug()<<"开启计时器";
    time->start();
    timer->start(1000);
}
//播放完毕，停止计时器
void MainWindow::timerStopText(){
    qDebug()<<"停止计时器";
    timer->stop();
    ui->timeButton->setText("0:0:0");
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
void MainWindow::slot_timer_timeout()
{
    qDebug()<<timeCounterString();
    ui->timeButton->setText(timeCounterString());
}
void MainWindow::on_playButton_clicked()
{
    if(_playThread){//停止播放
      _playThread->requestInterruption();
      _playThread = nullptr;
      ui->playButton->setText("开始播放");
    }else{//开始播放
        _playThread = new PlayThread(this);
        qDebug() << "_playThread的地址" << _playThread;
        connect(_playThread, &PlayThread::dataChanged, this,&MainWindow::changeButtonText);
        connect(_playThread,&PlayThread::startPlay,this,&MainWindow::timerStartText);
        connect(_playThread,&PlayThread::stopPlay,this,&MainWindow::timerStopText);
        _playThread->start();
        //监听线程的结束
        connect(_playThread,&PlayThread::finished,[this](){
                _playThread = nullptr;
                ui->playButton->setText("开始播放");
        });
        ui->playButton->setText("停止播放");
    }

}
