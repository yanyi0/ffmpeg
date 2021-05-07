#include "audiothread.h"
#include <QFile>
#include <QThread>
#include "ffmpegs.h"

extern "C" {
   //格式
   #include <libavformat/avformat.h>
}

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
    //44100_s16le_2 --> 48000_f32le_1 --> 48000_s32le_1 --> 44100_s16le_2
    //最后转化回来的44100_s16le_2，11,947,928 字节相比44100_s16le_2_new，11,947,932 字节会有误差，4个字节，因为向上兼容了缓冲区1个样本数量,一个样本占用4个字节16*2/8
    //若改为C语言,只改动文件操作fopen,fread,fwrite,fclose刷新缓冲区flush,缓冲区一般自动刷新，fclose也会刷新一次
    //输入参数
    ResampleAudioSpec ras1;
    ras1.filename = "/Users/cloud/Documents/iOS/音视频/TestMusic/Resample/44100_s16le_2.pcm";
    ras1.sampleFmt = AV_SAMPLE_FMT_S16;
    ras1.sampleRate = 44100;
    ras1.chLayout = AV_CH_LAYOUT_STEREO;

    //输出参数
    ResampleAudioSpec ras2;
    ras2.filename = "/Users/cloud/Documents/iOS/音视频/TestMusic/Resample/48000_f32le_1_new.pcm";
    ras2.sampleFmt = AV_SAMPLE_FMT_FLT;
    ras2.sampleRate = 48000;
    ras2.chLayout = AV_CH_LAYOUT_MONO;

    ResampleAudioSpec ras3;
    ras3.filename = "/Users/cloud/Documents/iOS/音视频/TestMusic/Resample/48000_s32le_1.pcm";
    ras3.sampleFmt = AV_SAMPLE_FMT_S32;
    ras3.sampleRate = 48000;
    ras3.chLayout = AV_CH_LAYOUT_MONO;

    ResampleAudioSpec ras4 = ras1;
    ras4.filename = "/Users/cloud/Documents/iOS/音视频/TestMusic/Resample/44100_s16le_2_new.pcm";
    FFmpegs::resampleAudio(ras1,ras2);
//    FFmpegs::resampleAudio(ras2,ras3);
//    FFmpegs::resampleAudio(ras3,ras4);
}


