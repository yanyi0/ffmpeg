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
}

#ifdef Q_QS_WIN
  //格式名称
  #define FMT_NAME "dshow"
  //设备名称
  #define DEVICE_NAME ""
  //PCM文件的文件名
  #define FILE_NAME "F:/out.pcm"
#else
  #define FMT_NAME "avfoundation"
  #define DEVICE_NAME ":0"
  #define FILE_NAME "/Users/cloud/Documents/iOS/音视频/TestMusic/RecordPcm/outxyzuvwsmnt.pcm"
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
void AudioThread::setStop(bool stop){
    _stop = stop;
}
void AudioThread::run(){
    qDebug() << this << "线程正常开始";
    AVInputFormat *fmt = av_find_input_format(FMT_NAME);
    if(!fmt){
        //如果找不到输入格式
        qDebug() << "找不到输入格式" << FMT_NAME;
        return;
    }

    //格式上下文(后面通过格式上下文操作设备)
    AVFormatContext *ctx = nullptr;

    //打开设备
    int ret = avformat_open_input(&ctx,DEVICE_NAME,fmt,nullptr);
    //若打开设备失败
    if(ret<0){
        char errorbuf[1024] = {0};
        //根据函数返回的错误码获取错误信息
        av_strerror(ret,errorbuf,sizeof(errorbuf));
        qDebug() << "打开设备失败" << errorbuf;
        return;
    }

    //文件
    QFile file(FILE_NAME);
    //WriteOnly:只写模式。如果文件不存在，就创建文件;如果文件存在，就删除文件内容
    if(!file.open(QFile::WriteOnly)){
        qDebug() << "文件打开失败" << FILE_NAME;
        //关闭设备
        avformat_close_input(&ctx);
        return;
    }

    //暂定只采集50个数据包,包太小可能导致时间太短50个包 205K,500个包 2M
//    int count = 500;
    //数据包
    AVPacket pkt;
    //从设备中采集数据
    //av_read_frame返回值为0，代表采集数据成功
    qDebug() << "开始采集数据" << pkt.size;
//    while(count > 0){
//    while(!_stop){
      while(!isInterruptionRequested()){
        ret = av_read_frame(ctx,&pkt);
        if( ret < 0){
            //-35
            if(ret == AVERROR(EAGAIN)){
                qDebug() << "采集数据错误-35";
                usleep(200);
                continue;
            }
        }else{
            qDebug() << "采集数据中" << pkt.size;
            //写入数据
            file.write((const char *)pkt.data,pkt.size);
//            count --;
        }
        av_packet_unref(&pkt);
    }
    //关闭文件
    file.close();
    //关闭设备
    avformat_close_input(&ctx);

    qDebug() << this << "线程正常结束";
}

