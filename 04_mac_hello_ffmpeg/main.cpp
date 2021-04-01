#include "mainwindow.h"
#include <QDebug>
#include <iostream>
#include <QApplication>
//由于ffmpeg是c开发的,所以需要加上extern "C" 关键字
extern "C"{
#include <libavcodec/avcodec.h>
};

int main(int argc, char *argv[])
{
    qDebug() << av_version_info();
    qDebug() << av_cpu_count();
//    std::cout << av_version_info();
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
