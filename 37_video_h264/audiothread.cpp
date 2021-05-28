#include "audiothread.h"
#include <QFile>
#include <QThread>
#include "ffmpegs.h"

extern "C" {
   //格式
   #include <libavformat/avformat.h>
}

#ifdef Q_OS_WIN
#define INFILENAME "D:/音视频/TestMusic/H264_Code/out.yuv"
#define OUTFILENAME "D:/音视频/TestMusic/H264_Code/out_win_nopixelfromat.h264"
#else
#define INFILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/H264_Code/out.yuv"
#define OUTFILENAME ""/Users/cloud/Documents/iOS/音视频/TestMusic/H264_Code/out_nopixelformat.h264""
#endif

AudioThread::AudioThread(QObject *parent) : QThread(parent)
{
    //监听到线程结束，自己调用delete
     connect(this,&AudioThread::finished,this,&AudioThread::deleteLater);
}
AudioThread::~AudioThread(){
    //内存回收之前 监听到内存要回收，结束线程
    requestInterruption();
    quit();
    wait();
   qDebug() << "AudioThread::~AudioThread()" << "子线程释放 内存被回收";
}
//此句代码释放内存无效，只释放了形参的内存
//void freep(void *p){
//    free(p);
//    p = nullptr;
//}
//能真正释放开辟的内存空间
void freep(void **p){
    free(*p);
    *p = nullptr;
}
void freeResources(){
    //释放开辟的内存空间
    void *ptr = malloc(100);
    free(ptr);
    ptr = nullptr;
    //此句代码释放内存无效，只释放了形参的内存
 //    freep(ptr);
    //能真正释放开辟的内存空间
    freep(&ptr);
}

void AudioThread::run(){
    //输入参数
    VideoEncodeSpec in;
    in.filename = INFILENAME;
    //注意windows默认不传值的情况下，是YUV420p亲测有效，后面的流程正常执行会生成H264文件，因为AV_PIX_FMT_YUV420P的值默认为0
    //而0正好是AV_PIX_FMT_YUV420P，Mac上不传的值为随机值251498416，这个随机值的格式，FFMpeg不存在，故不传会报错
//    in.pixelFmt = AV_PIX_FMT_YUV420P;
    in.width = 768;
    in.height = 432;
    in.fps = 23;
    FFmpegs::h264Encode(in,OUTFILENAME);
}


