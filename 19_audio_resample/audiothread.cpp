#include "audiothread.h"
#include <QFile>
#include <QThread>


extern "C" {
   //设备
   #include <libavdevice/avdevice.h>
   //格式
   #include <libavformat/avformat.h>
   //工具
   #include <libavutil/avutil.h>
   //重采样
   #include <libswresample/swresample.h>
}

#define AV_ERROR(ret) \
    char errorbuf[1024] = {0};\
    av_strerror(ret,errorbuf,sizeof(errorbuf));

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
   //文件名
   const char *inFilename = "/Users/cloud/Documents/iOS/音视频/TestMusic/Resample/44100_2_f32le.pcm";
   QFile inFile(inFilename);
   const char *outFilename = "/Users/cloud/Documents/iOS/音视频/TestMusic/Resample/44100_2_sl6le.pcm";
   QFile outFile(outFilename);
   //输入参数
   AVSampleFormat inSampleFmt = AV_SAMPLE_FMT_FLT;
   int inSampleRate = 44100;
   int inChLayout = AV_CH_LAYOUT_STEREO;

   //创建输入缓冲区
   //指向缓冲区的指针
   uint8_t **inData = nullptr;
   //缓冲区的大小
   int inLinesize = 0;
   //声道数
   int inChs = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
   //每个输入样本的大小
   int inBytesPerSample = inChs * av_get_bytes_per_sample(inSampleFmt);
   //缓冲区的样本数量
   int inSamples = 1024;
   //读取文件数据的大小
   int len = 0;

   //输出参数
   AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_S16;
   int outSampleRate = 44100;
   int outChLayout = AV_CH_LAYOUT_STEREO;
   //创建输出缓冲区
   //指向缓冲区的指针
   uint8_t **outData = nullptr;
   //缓冲区的大小
   int outLinesize = 0;
   //声道数
   int outChs = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
   //每个输入样本的大小
   int outBytesPerSample = outChs * av_get_bytes_per_sample(outSampleFmt);
   //缓冲区的样本数量
   int outSamples = 1024;
   //返回结果
   int ret = 0;
   //创建重采样上下文
   SwrContext *ctx = swr_alloc_set_opts(nullptr,
                                        //输出参数
                                        outChLayout,outSampleFmt,outSampleRate,
                                        //输入参数
                                        inChLayout,inSampleFmt,inSampleRate,
                                        0,nullptr);
   if(!ctx){
       qDebug() << "swr_alloc_set_opts";
       return;
   }

   //初始化重采样上下文
   ret = swr_init(ctx);
   if(ret < 0){
       //根据函数返回的错误码获取错误信息
       AV_ERROR(ret);
       qDebug() << "swr_init error" << errorbuf;
       goto end;
   }
   //创建输入缓冲区
   ret = av_samples_alloc_array_and_samples(&inData,&inLinesize,inChs,inSamples,inSampleFmt,1);
   if(ret < 0){
       AV_ERROR(ret);
       qDebug() << "av_samples_alloc_array_and_samples error" << errorbuf;
       goto end;
   }
   //创建输出缓冲区
   ret = av_samples_alloc_array_and_samples(&outData,&outLinesize,outChs,outSamples,outSampleFmt,1);
   if(ret < 0){
       AV_ERROR(ret);
       qDebug() << "av_samples_alloc_array_and_samples error" << errorbuf;
       goto end;
   }

   //打开文件
   if(!inFile.open(QFile::ReadOnly)){
       qDebug() << "inFile open error" << inFilename;
       goto end;
   }
   if(!outFile.open(QFile::WriteOnly)){
       qDebug() << "outFile open error" << outFilename;
       goto end;
   }

   //读取文件数据
   while((len = inFile.read((char *) inData[0],inLinesize)) > 0){
       //读取的输入缓冲区的样本大小
       inSamples = len / inBytesPerSample;
       //重采样(返回值为重采样后的样本数量)
       ret = swr_convert(ctx,outData,outSamples,(const uint8_t **)inData,inSamples);
       if(ret < 0){
           AV_ERROR(ret);
           qDebug() << "swr_convert error" << errorbuf;
           goto end;
       }
       //将转换后的数据写入到输出文件中
       outFile.write((char *)outData[0],ret*outBytesPerSample);
   }
end:
   //释放资源
   inFile.close();
   outFile.close();
   //释放输入缓冲区
   if(inData){
       av_freep(&inData[0]);
   }
   av_freep(&inData);
   //释放输出缓冲区
   if(outData){
       av_freep(&outData[0]);
   }
   av_freep(&outData);
   //释放重采样上下文
   swr_free(&ctx);

}



