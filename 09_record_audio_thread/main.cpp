#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QThread>

extern "C" {
//设备
#include <libavdevice/avdevice.h>
}

int main(int argc, char *argv[])
{
    qDebug() << "main" << QThread::currentThread();
    //注册设备
    avdevice_register_all();
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
