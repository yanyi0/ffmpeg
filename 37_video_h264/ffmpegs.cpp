#include "ffmpegs.h"
#include "QDebug"
extern "C" {
#include <libavutil/imgutils.h>
}
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
//检查编码器codec是否支持像素格式
static int check_pixel_fmt(const AVCodec *codec,enum AVPixelFormat pixelFmt){
    const enum AVPixelFormat *p = codec->pix_fmts;
    while (*p != AV_PIX_FMT_NONE) {
        //打印出codec支持的所有采样格式fmt类型 av_get_sample_fmt_name s16
        //不支持的采样格式可以通过音频重采样进行转换
        qDebug() << "av_get_pix_fmt_name" << av_get_pix_fmt_name(*p);
        if(*p == pixelFmt) return 1;
        p++;
    }
    return 0;
}

void FFmpegs::h264Encode(VideoEncodeSpec &in, const char *outFilename){
    //编码器
    AVCodec *codec = nullptr;
    //上下文
    AVCodecContext *ctx = nullptr;
    //一帧图片的大小
    int imgSize = av_image_get_buffer_size(in.pixelFmt,in.width,in.height,1);
    //用来存放编码前的数据(yuv)
    AVFrame *frame = nullptr;
    //用来存放编码后的数据(h264)
    AVPacket *packet = nullptr;
    //返回结果
    int ret = 0;
    //输入文件
    QFile inFile(in.filename);
    //输出文件
    QFile outFile(outFilename);
    //获取编码器
    codec = avcodec_find_encoder_by_name("libx264");
    if(!codec){
        qDebug() << "encoder libfdk_aac not found";
        return;
    }
    //检查编码器是否支持当前的采样格式
    if(!check_pixel_fmt(codec,in.pixelFmt)){
        qDebug() << "Encoder does not support pixel format" << av_get_pix_fmt_name(in.pixelFmt);
        return;
    }
    //创建上下文
    ctx = avcodec_alloc_context3(codec);
    if(!ctx){
       qDebug() << "avcodec_alloc_context3 error";
       return;
    }
    //设置yuv参数
    ctx->pix_fmt = in.pixelFmt;
    ctx->width = in.width;
    ctx->height = in.height;
    //设置帧率，一秒钟显示的帧数是in.fps
    ctx->time_base = {1,in.fps};
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
    frame->format = ctx->pix_fmt;
    frame->width = ctx->width;
    frame->height = ctx->height;
    frame->pts = 0;
    //查看帧缓冲区的大小
    qDebug() << "帧缓冲区大小" << frame->linesize[0];
    //利用format，width,height创建AVFrame内部的缓冲区
    ret = av_image_alloc(frame->data,frame->linesize,in.width,in.height,in.pixelFmt,1);
    qDebug() << "帧缓冲区大小" << frame->linesize[0];
    if(ret < 0){
        AV_ERROR(ret);
        qDebug() << "av_frame_get_buffer error" << errorbuf;
        goto end;
    }
//    ret = av_frame_get_buffer(frame,0);
//    qDebug() << "帧缓冲区大小" << frame->linesize[0];
//    if(ret < 0){
//        AV_ERROR(ret);
//        qDebug() << "av_frame_get_buffer error" << errorbuf;
//        goto end;
//    }
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
    //读取数据到frame中,每次读取一帧图片的大小
    while((ret = inFile.read((char *) frame->data[0],imgSize))>0){
        //进行编码
        if(encode(ctx,frame,packet,outFile)<0){
            goto end;
        }
        frame->pts++;
    }
    //刷新缓冲区
    encode(ctx,nullptr,packet,outFile);
end:
    //关闭文件
    inFile.close();
    outFile.close();
    //释放资源
    //frame存在采取释放data[0]指向的区域
    if(frame){
        av_freep(&frame->data[0]);
        av_frame_free(&frame);
    }
    av_packet_free(&packet);
    avcodec_free_context(&ctx);
}

