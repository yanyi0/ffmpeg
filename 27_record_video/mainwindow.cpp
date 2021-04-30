#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"
#include <QThread>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug() << this << "主线程正常开始";
    ui->setupUi(this);
    qDebug() << "主窗口 主线程 MainWindow" << QThread::currentThread();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_audioButton_clicked()
{
    qDebug() << "点击按钮 on_audioButton_clicked" << QThread::currentThread();
    if(_audioThread == nullptr){//点击了开始录音
        qDebug() << "开始录视频";
        //开启线程
        _audioThread = new AudioThread(this);
        _audioThread->start();
        //线程非正常退出，按钮文字改为开始录音，同时指针置为nil，防止下次点击访问空指针对象
        connect(_audioThread,&AudioThread::finished,[this](){
           //线程结束
            ui->audioButton->setText("开始录视频");
            ui->audioButton->repaint();
            _audioThread = nullptr;
        });
        //设置按钮文字
        ui->audioButton->setText("结束录视频");
        ui->audioButton->repaint();
      }else{//点击了结束录音
         //结束线程
//        _audioThread->setStop(true);
        _audioThread->requestInterruption();
        _audioThread = nullptr;
        //设置按钮文字
        ui->audioButton->setText("开始录视频");
        ui->audioButton->repaint();
      }
}
