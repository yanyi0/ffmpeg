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
        qDebug() << "开始录音";
        //设置按钮文字
        ui->audioButton->setText("结束录音");
        //开启线程
        _audioThread = new AudioThread(this);
        _audioThread->start();

      }else{//点击了结束录音
        //设置按钮文字
        ui->audioButton->setText("开始录音");
         //结束线程
//        _audioThread->setStop(true);
        _audioThread->requestInterruption();
        _audioThread = nullptr;
      }

}
