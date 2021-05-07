#include "ffmpegs.h"
#include "QDebug"
FFmpegs::FFmpegs()
{

}
static int encode(AVCodecContext *ctx,AVFrame *frame,AVPacket *pkt,QFile &outFile){
     //发送数据到编码器
    int ret = avcodec_send_frame(ctx,frame);
    if(ret<0){
         AV_ERROR(ret);
         qDebug() << "avcodec_send_frame error" << errorbuf;
         return ret;
    }
    while(true){
        //从编码器中获取编码后的数据
        ret = avcodec_receive_packet(ctx,pkt);
        //packet中已经没有数据，需要重新发送数据到编码器(send frame)
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){//出现了其他错误
            AV_ERROR(ret);
            qDebug() << "avcodec_send_frame error" << errorbuf;
            return ret;
        }
        //成功从编码器拿到编码后的数据，将编码后的数据写入文件
        outFile.write((char *)pkt->data,pkt->size);
        av_packet_unref(pkt);
    }
}
//检查编码器codec是否支持采样格式sample_fmt
static int check_sample_fmt(const AVCodec *codec,enum AVSampleFormat sample_fmt){
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        //打印出codec支持的所有采样格式fmt类型 av_get_sample_fmt_name s16
        //不支持的采样格式可以通过音频重采样进行转换
        qDebug() << "av_get_sample_fmt_name" << av_get_sample_fmt_name(*p);
        if(*p == sample_fmt) return 1;
        p++;
    }
    return 0;
}

void FFmpegs::audioEncode(AudioEncodeSpec &in, const char *outFilename){
    //编码器
    AVCodec *codec = nullptr;
    //上下文
    AVCodecContext *ctx = nullptr;
    //用来存放编码前的数据
    AVFrame *frame = nullptr;
    //用来存放编码后的数据
    AVPacket *packet = nullptr;
    //返回结果
    int ret = 0;
    //输入文件
    QFile inFile(in.filename);
    //输出文件
    QFile outFile(outFilename);
    //获取编码器
    AVCodec *codec1 = avcodec_find_encoder(AV_CODEC_ID_AAC);
    AVCodec *codec2 = avcodec_find_encoder_by_name("aac");
    //true，ffmpeg自带的音频编码器
    qDebug() << (codec1 == codec2);
    qDebug() << codec1->name;
    //获取libfdk_aac编码器
    codec = avcodec_find_encoder_by_name("libfdk_aac");
    if(!codec){
        qDebug() << "encoder libfdk_aac not found";
        return;
    }
    //检查编码器是否支持当前的采样格式
    if(!check_sample_fmt(codec,in.sampleFmt)){
        qDebug() << "Encoder does not support sample format" << av_get_sample_fmt_name(in.sampleFmt);
        return;
    }
    //创建上下文
    ctx = avcodec_alloc_context3(codec);
    if(!ctx){
       qDebug() << "avcodec_alloc_context3 error";
       return;
    }
    //设置参数
    ctx->sample_fmt = in.sampleFmt;
    ctx->sample_rate = in.sampleRate;
    ctx->channel_layout = in.chLayout;
    //比特率
    ctx->bit_rate = 32000;
    //规格
    ctx->profile = FF_PROFILE_AAC_HE_V2;

    //var参数设置
//    AVDictionary *options = nullptr;
//    av_dict_set(&options,"vbr","5",0);
//    ret = avcodec_open2(ctx,codec,&options);
    //打开编码器
    ret = avcodec_open2(ctx,codec,nullptr);
    if(ret < 0){
        AV_ERROR(ret);
        qDebug() << "avcodec_open2 error" << errorbuf;
        goto end;
    }
    //创建AVFrame
    frame = av_frame_alloc();
    if(!frame){
        qDebug() << "av_frame_alloc error";
        goto end;
    }
    //样本帧数量(由frame_size决定)
    frame->nb_samples = ctx->frame_size;
    qDebug() << "样本帧数量" << frame->nb_samples;
    //采样格式,每个样本帧的大小
    frame->format = ctx->sample_fmt;
    qDebug() << "采样格式" << frame->format << av_get_bytes_per_sample(ctx->sample_fmt) << av_get_bits_per_sample(AV_CODEC_ID_PCM_S16LE);
    //声道布局
    frame->channel_layout = ctx->channel_layout;
    qDebug() << "声道布局" << frame->channel_layout << "声道数量" << av_get_channel_layout_nb_channels(frame->channel_layout);
    //查看帧缓冲区的大小
    qDebug() << "帧缓冲区大小" << frame->linesize[0];
    //创建AVFrame内部的缓冲区
    ret = av_frame_get_buffer(frame,0);
    // 每个声道的样本数量 frame->nb_samples * 声道数 av_get_channel_layout_nb_channels(frame->channel_layout) * 每个样本的位深度字节数
    // 2048 * 2 * 2 = 8196
    qDebug() << "帧缓冲区大小" << frame->linesize[0];
    if(ret < 0){
        AV_ERROR(ret);
        qDebug() << "av_frame_get_buffer error" << errorbuf;
        goto end;
    }
    //创建AVPacket
    packet = av_packet_alloc();
    if(!packet){
        qDebug() << "av_packet_alloc error";
        goto end;
    }
    //打开文件
    if(!inFile.open(QFile::ReadOnly)){
        qDebug() << "file open error" << in.filename;
        goto end;
    }
    if(!outFile.open(QFile::WriteOnly)){
        qDebug() << "file open error" << outFilename;
        goto end;
    }
    //开始编码
    //读取数据到frame中
    while((ret = inFile.read((char *) frame->data[0],frame->linesize[0]))>0){
        //最后一次读取数据，无法全部充满帧缓冲区，实际读取多少，写入多少到输出文件中
        if(ret < frame->linesize[0]){
            //声道数
            int channels = av_get_channel_layout_nb_channels(frame->channel_layout);
            //每个样本的大小
            int perSampleSize = av_get_bytes_per_sample(ctx->sample_fmt);
            //总共的样本数量
            frame->nb_samples = ret/(channels * perSampleSize);
        }
        //进行编码
        if(encode(ctx,frame,packet,outFile)<0){
            goto end;
        }
    }
    //刷新缓冲区
    encode(ctx,nullptr,packet,outFile);
end:
    //关闭文件
    inFile.close();
    outFile.close();
    //释放资源
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&ctx);
}

