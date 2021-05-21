#include "mainwindow.h"
#include <QApplication>
#include "ffmpegs.h"

int main(int argc, char *argv[])
{

    //文件转换
//    RawVideoFile in = {
//        "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/outyuv420p.yuv",512,512,AV_PIX_FMT_YUV420P
//    };
//    RawVideoFile out = {
//        "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/out_640x640_yuyv422.yuv",640,640,AV_PIX_FMT_YUYV422
//    };
//    FFmpegs::convertVideoRaw(in,out);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
