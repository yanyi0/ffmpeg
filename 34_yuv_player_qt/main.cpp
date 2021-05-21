#include "mainwindow.h"
#include <QApplication>
#include "ffmpegs.h"

int main(int argc, char *argv[])
{
    /*
    //文件转换
    RawVideoFile in = {
        "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/out1_yuyv422_1280x720.yuv",1280,720,AV_PIX_FMT_YUYV422
    };
    RawVideoFile out = {
        "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/out_640x640_nv42.yuv",640,640,AV_PIX_FMT_NV42
    };
    FFmpegs::convertVideoRaw(in,out);
    */
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
