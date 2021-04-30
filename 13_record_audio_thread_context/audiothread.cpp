#include "audiothread.h"
#include <QFile>
#include <QThread>
#include <QTime>
#include <QDebug>

extern "C" {
   //设备
   #include <libavdevice/avdevice.h>
   //格式
   #include <libavformat/avformat.h>
   //工具
   #include <libavutil/avutil.h>
}

#ifdef Q_OS_WIN32
  //格式名称
  #define FMT_NAME "dshow"
  //设备名称
  #define DEVICE_NAME "麦克风阵列 (IDT High Definition Audio CODEC)"
  //PCM文件的文件名
  #define FILE_NAME "D:/音视频/TestMusic/RecordToPlay/out.pcm"
  //生成文件路径
  #define FILE_PATH "D:/音视频/TestMusic/RecordToPlay/"
#else
  #define FMT_NAME "avfoundation"
  #define DEVICE_NAME ":0"
  #define FILE_NAME "/Users/cloud/Documents/iOS/音视频/TestMusic/outoptimize.pcm"
  #define FILE_PATH "/Users/cloud/Documents/iOS/音视频/TestMusic/RecordPcm/"
#endif

AudioThread::AudioThread(QObject *parent) : QThread(parent)
{
    //监听到线程结束，自己调用delete
     connect(this,&AudioThread::finished,this,&AudioThread::deleteLater);
}
AudioThread::~AudioThread(){
    //断开所有的连接
    disconnect();
    //内存回收之前 监听到内存要回收，结束线程
    requestInterruption();
    quit();
    wait();
   qDebug() << "AudioThread::~AudioThread()" << "子线程释放 内存被回收";
}
void AudioThread::setStop(bool stop){
    _stop = stop;
}

void showSpec(AVFormatContext *context){
    //获取输入流
    AVStream *stream = context->streams[0];
    //获取音频参数
    AVCodecParameters *params = stream->codecpar;
    //声道数
    qDebug() << params->channels;
    //采样率
    qDebug() << params->sample_rate;
    //采样格式 16 * 2/8 = 4字节
    qDebug() << params->format;//获取为-1
    //每一个样本的一个声道占用多少个字节
    qDebug() << "av_get_bytes_per_sample" << av_get_bytes_per_sample((AVSampleFormat) params->format);
    //采样格式
    qDebug() <<  "params->codec_id" << params->codec_id;
    //采样格式name pcm_f32le
    qDebug() << "avcodec_get_name" << avcodec_get_name(params->codec_id);
    qDebug() << "av_get_sample_fmt" << av_get_sample_fmt(avcodec_get_name(params->codec_id));
    //每一个样本的一个声道占用多少个字节
    qDebug() << "av_get_bits_per_sample" << av_get_bits_per_sample(params->codec_id);
}
void AudioThread::run(){
    qDebug() << this << "线程正常开始----------------";
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

    //打印一下录音设备的参数信息
    showSpec(ctx);

    //文件名
//    std::string str = FILE_PATH;
//    str += ".pcm";
    QString  filename = FILE_PATH;
    filename += QDateTime::currentDateTime().toString("yyyy-MM-dd_hh_mm_ss");
    filename += ".pcm";

    //文件
    QFile file(filename);
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
      while(!isInterruptionRequested()){
        ret = av_read_frame(ctx,&pkt);
        if( ret == 0){//读取成功
//            qDebug() << "采集数据中" << pkt.size;
            //写入数据
            file.write((const char *)pkt.data,pkt.size);
        }else if (ret == AVERROR(EAGAIN)){
            //-35
//            qDebug() << "采集数据错误-35";
            continue;
        }else{
            char errorbuf[1024] = {0};
            //根据函数返回的错误码获取错误信息
            av_strerror(ret,errorbuf,sizeof(errorbuf));
            qDebug() << "av_read_frame error" << errorbuf << ret;
        }
    }
    //关闭文件
    file.close();
    //关闭设备
    avformat_close_input(&ctx);

    qDebug() << this << "线程正常结束";
}

