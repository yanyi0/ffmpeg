#include "audiothread.h"
#include <QFile>
#include <QThread>
#include <QTime>
#include "ffmpegs.h"

extern "C" {
   //设备
   #include <libavdevice/avdevice.h>
   //格式
   #include <libavformat/avformat.h>
   //工具
   #include <libavutil/avutil.h>
}
//采样率
#define SAMPLERATE 44100
//声道
#define NUMBERCHANNELS 2
#ifdef Q_QS_WIN
  //格式名称
  #define FMT_NAME "dshow"
  //设备名称
  #define DEVICE_NAME ""
  //PCM文件的文件名
  #define FILE_NAME "F:/out.pcm"
  #define BITSPERSAMPLE 16
#else
  #define FMT_NAME "avfoundation"
  #define DEVICE_NAME ":0"
  #define FILE_NAME "/Users/cloud/Documents/iOS/音视频/TestMusic/outoptimize.pcm"
  #define FILE_PATH "/Users/cloud/Documents/iOS/音视频/TestMusic/RecordWav/"
  #define BITSPERSAMPLE 32
  #define PCMFILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PcmToWav/MyHeartWillGoOnLoavAll.pcm"
  #define WAVFILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PcmToWav/MyHeartWillGoOnLoavAll.wav"
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

    //文件名
    QString  wavFilename = FILE_PATH;
    wavFilename += QDateTime::currentDateTime().toString("yyyy-MM-dd_hh_mm_ss");
    wavFilename += ".wav";

    //文件
    QFile wavFile(wavFilename);
    //WriteOnly:只写模式。如果文件不存在，就创建文件;如果文件存在，就删除文件内容
    if(!wavFile.open(QFile::ReadWrite)){
        qDebug() << "文件打开失败" << wavFilename;
        //关闭设备
        avformat_close_input(&ctx);
        return;
    }

    //获取设备参数
    //获取输入流
    AVStream *stream = ctx->streams[0];
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

    //pcm 转 wav
    WAVHeader wavHead;

    qDebug() << sizeof (WAVHeader);
    //声道 2
    wavHead.nubmerChannels =params->channels;
    //采样率
    wavHead.sampleRate = params->sample_rate;
    //位深度
    wavHead.bitsPerSample =  av_get_bits_per_sample(params->codec_id);
    //一个样本所占的字节数
    wavHead.blockAlign = wavHead.bitsPerSample* wavHead.nubmerChannels >> 3;
    //字节率
    wavHead.byteRate = params->sample_rate * wavHead.blockAlign;
    //浮点型为3，PCM为1
    wavHead.audioFormat = (params->codec_id >= AV_CODEC_ID_PCM_F32BE) ? 3 : 1;

    wavFile.write((const char *)&wavHead,sizeof(WAVHeader));
    //数据包
    AVPacket pkt;
    //从设备中采集数据
    //av_read_frame返回值为0，代表采集数据成功
    qDebug() << "开始采集数据" << pkt.size;
      while(!isInterruptionRequested()){
        ret = av_read_frame(ctx,&pkt);
        if( ret == 0){//读取成功
            //qDebug() << "采集数据中" << pkt.size;
            //写入数据
            wavFile.write((const char *)pkt.data,pkt.size);
        }else if (ret == AVERROR(EAGAIN)){
            continue;
        }else{
            char errorbuf[1024] = {0};
            //根据函数返回的错误码获取错误信息
            av_strerror(ret,errorbuf,sizeof(errorbuf));
            qDebug() << "av_read_frame error" << errorbuf << ret;
        }
        //必须要加，释放pkt内部资源，比如pkt->data,pkt->buf
        av_packet_unref(&pkt);
      }

      int allSize = wavFile.size();
      qDebug() << "文件总大小-------------" << allSize;
      //文件总大小减8
      int chunkDataSize = allSize - sizeof(wavHead.riffChunkId) - sizeof(wavHead.chunkDataSize);
      qDebug() << "chunkDataSize大小-------------" << chunkDataSize;
      //dataChunkDataSize
      int dataChunkDataSize = allSize - sizeof(wavHead);
      qDebug() << "dataChunkDataSize大小-------------" << dataChunkDataSize;


      if(wavFile.seek(sizeof(wavHead.riffChunkId)) == true){

//           QByteArray abyte0;
//           abyte0.resize(4);
//           abyte0[0] = (uchar)  (0x000000ff & chunkDataSize);
//           abyte0[1] = (uchar) ((0x0000ff00 & chunkDataSize) >> 8);
//           abyte0[2] = (uchar) ((0x00ff0000 & chunkDataSize) >> 16);
//           abyte0[3] = (uchar) ((0xff000000 & chunkDataSize) >> 24);

//           qDebug() << abyte0;

//           qDebug() << abyte0.toHex();

//           wavFile.write(abyte0);

          wavFile.write((char *)&chunkDataSize,sizeof(chunkDataSize));

           if(wavFile.seek(sizeof(WAVHeader) - sizeof(wavHead.dataChunkDataSize))){
//               QByteArray abyte1;
//               abyte1.resize(4);
//               abyte1[0] = (uchar)  (0x000000ff & dataChunkDataSize);
//               abyte1[1] = (uchar) ((0x0000ff00 & dataChunkDataSize) >> 8);
//               abyte1[2] = (uchar) ((0x00ff0000 & dataChunkDataSize) >> 16);
//               abyte1[3] = (uchar) ((0xff000000 & dataChunkDataSize) >> 24);

//               qDebug() << abyte1;

//               qDebug() << abyte1.toHex();

//               wavFile.write(abyte1);

                 wavFile.write((char *)&dataChunkDataSize,sizeof(dataChunkDataSize));
           }
      }


      //关闭文件
      wavFile.close();

      qDebug() << this << "线程正常结束";

      //关闭设备
      avformat_close_input(&ctx);
}

