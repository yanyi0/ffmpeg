#include "audiothread.h"
#include <QFile>
#include <QThread>
#include "demuxer.h"
#include "ffmpegs.h"
extern "C" {
   //格式
   #include <libavutil/imgutils.h>
}
#ifdef Q_OS_WIN
#define INFILENAME "D:/音视频/TestMusic/H264_Decode/out.h264"
#define OUTFILENAME "D:/音视频/TestMusic/H264_Decode/out_win.yuv"
#else
#define INFILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/Demux/in.mp4"
#define AOUTFILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/Demux/out_code.pcm"
#define VOUTFILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/Demux/out_code.yuv"
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
    //解封装后的音频数据
    AudioDecodeSpec aOut;
    aOut.filename = AOUTFILENAME;
    //解封装后的视频数据
    VideoDecodeSpec vOut;
    vOut.filename = VOUTFILENAME;

    Demuxer().demux(INFILENAME,aOut,vOut);

    qDebug() << "输出音频:" << aOut.sampleRate << av_get_channel_layout_nb_channels(aOut.chLayout) << av_get_sample_fmt_name(aOut.sampleFmt);
    qDebug() << "输出视频:" << vOut.width << vOut.height << vOut.fps << av_get_pix_fmt_name(vOut.pixFmt);
}


