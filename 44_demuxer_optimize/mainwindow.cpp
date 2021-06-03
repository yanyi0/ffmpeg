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
    //开启线程
    _audioThread = new AudioThread(this);
    _audioThread->start();
}
