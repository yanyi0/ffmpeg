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
   #include <libavutil/imgutils.h>
}
//采样率
#define SAMPLERATE 44100
//声道
#define NUMBERCHANNELS 2
#ifdef Q_OS_WIN32
  //格式名称
  #define FMT_NAME "dshow"
  //设备名称
  #define DEVICE_NAME "video=Integrated Webcam"
  //YUV文件的文件名
  #define FILENAME "D:/音视频/TestMusic/RecordVideo/record640x480yuv.yuv"
#else
  #define FMT_NAME "avfoundation"
  #define DEVICE_NAME "0"
  #define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/RecordVideo/out2.yuv"
#endif

#define ERROR_BUF(ret) \
    char errbuf[1024]; \
    av_strerror(ret,errbuf,sizeof(errbuf));

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

//打印一下录音设备的参数信息
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
        qDebug() << "av_find_input_format error" << FMT_NAME;
        return;
    }

    //格式上下文(后面通过格式上下文操作设备)
    AVFormatContext *ctx = nullptr;
    //设备参数
    AVDictionary *options = nullptr;
    av_dict_set(&options,"video_size","640x480",0);
    av_dict_set(&options,"pixel_format","yuyv422",0);
    av_dict_set(&options,"framerate","30",0);
    //打开设备
    int ret = avformat_open_input(&ctx,DEVICE_NAME,fmt,&options);
    //若打开设备失败
    if(ret<0){
        //根据函数返回的错误码获取错误信息
        ERROR_BUF(ret);
        qDebug() << "打开设备失败" << errbuf;
        return;
    }

    //文件
    QFile file(FILENAME);
    //WriteOnly:只写模式。如果文件不存在，就创建文件;如果文件存在，就删除文件内容
    if(!file.open(QFile::ReadWrite)){
        qDebug() << "file open error" << FILENAME;
        //关闭设备
        avformat_close_input(&ctx);
        return;
    }

    //计算一帧的大小
    AVCodecParameters *params = ctx->streams[0]->codecpar;
    AVPixelFormat pixFmt = (AVPixelFormat)params->format;
    int imageSize = av_image_get_buffer_size(pixFmt,params->width,params->height,1);
//    qDebug() << pixFmt << params->width << params->height;
//    int pixSize = av_get_bits_per_pixel(av_pix_fmt_desc_get(pixFmt)) >> 3;
//    qDebug() << av_pix_fmt_desc_get(pixFmt)->name << pixSize;
//    int imageSize = params->width * params->height * pixSize;
    qDebug() << imageSize;

    //数据包
    AVPacket *pkt = av_packet_alloc();
    //从设备中采集数据
    //av_read_frame返回值为0，代表采集数据成功
    qDebug() << "开始采集数据" << pkt->size;
    while(!isInterruptionRequested()){
        ret = av_read_frame(ctx,pkt);
        if( ret == 0){//读取成功
            //qDebug() << "采集数据中" << pkt.size;
            //写入数据
            file.write((const char *)pkt->data,imageSize);
            qDebug() << pkt->size;
            //释放资源
            av_packet_unref(pkt);
        }else if (ret == AVERROR(EAGAIN)){
            continue;
        }else{
            //根据函数返回的错误码获取错误信息
            ERROR_BUF(ret);
            qDebug() << "av_read_frame error" << errbuf << ret;
        }
        //必须要加，释放pkt内部资源，比如pkt->data,pkt->buf
        av_packet_unref(pkt);
    }
      //释放资源
      av_packet_free(&pkt);
      //关闭文件
      file.close();

      qDebug() << this << "线程正常结束";
      //关闭设备
      avformat_close_input(&ctx);
}

