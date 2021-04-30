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
    AudioEncodeSpec in;
    in.filename = "/Users/cloud/Documents/iOS/音视频/TestMusic/AAC_Code/44100_s16le_2.pcm";
    in.sampleFmt = AV_SAMPLE_FMT_S16;
    in.sampleRate = 44100;
    in.chLayout = AV_CH_LAYOUT_STEREO;
    FFmpegs::audioEncode(in,"/Users/cloud/Documents/iOS/音视频/TestMusic/AAC_Code/out_set32_HE-AACv2_optimize.aac");
}


