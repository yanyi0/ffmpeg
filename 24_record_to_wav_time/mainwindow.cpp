#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"
#include <QThread>
#include <QTime>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug() << this << "主线程正常开始";
    ui->setupUi(this);
    qDebug() << "主窗口 主线程 MainWindow" << QThread::currentThread();
    //初始化时间
    onTimeChanged(0);
    //timeLabel设置为等宽字体，不会忽长忽短变化,每个数字的宽度相同，不会因为0宽一些，1窄一些
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::onTimeChanged(unsigned long long ms){
    QTime time(0,0,0,0);
    //分:秒.毫秒  02:03.789
    QString text = time.addMSecs(ms).toString("mm:ss.z");
    //取前面七位 02:03.7
    ui->timeLabel->setText(text.left(7));
}
void MainWindow::on_audioButton_clicked()
{
    qDebug() << "点击按钮 on_audioButton_clicked" << QThread::currentThread();
    if(_audioThread == nullptr){//点击了开始录音
        qDebug() << "开始录音";
        //开启线程
        _audioThread = new AudioThread(this);
        _audioThread->start();
        connect(_audioThread,&AudioThread::timeChange,this,&MainWindow::onTimeChanged);
        //线程非正常退出，按钮文字改为开始录音，同时指针置为nil，防止下次点击访问空指针对象
        connect(_audioThread,&AudioThread::finished,[this](){
           //线程结束
            this->ui->audioButton->setText("开始录音");
            ui->audioButton->repaint();
            this->_audioThread = nullptr;
        });
        //设置按钮文字
        ui->audioButton->setText("结束录音");
        ui->audioButton->repaint();
      }else{//点击了结束录音
         //结束线程
//        _audioThread->setStop(true);
        _audioThread->requestInterruption();
        _audioThread = nullptr;
        //设置按钮文字
        ui->audioButton->setText("开始录音");
        ui->audioButton->repaint();
      }
}
