#include "ffmpegs.h"
#include "QDebug"
//输入缓冲区的大小
#define IN_DATA_SIZE 20480
//需要再次读取文件数据的阀值
#define REFILL_THRESH 4096
FFmpegs::FFmpegs()
{

}
static int decode(AVCodecContext *ctx,AVPacket *pkt,AVFrame *frame,QFile &outFile){
    //发送压缩数据到解码器
    int ret = avcodec_send_packet(ctx,pkt);
    if(ret < 0){
        AV_ERROR(ret);
        qDebug() << "avcodec_send_packet error" << errorbuf;
        return ret;
    }
    while(true){
        //获取解码后的数据
        ret = avcodec_receive_frame(ctx,frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return 0;
        }else if(ret < 0){
            AV_ERROR(ret);
            qDebug() << "avcodec_receive_frame error" << errorbuf;
            return ret;
        }
        //将解码后的数据写入文件·
        outFile.write((char *)frame->data[0],frame->linesize[0]);
    }
}
void FFmpegs::audioDecode(const char *inFilename,AudioDecodeSpec &out){

    //返回结果
    int ret = 0;
    //每次从文件中读取的长度
    int inLen = 0;

    int inEnd = 0;
    //用来存放读取的文件数据
    //加上AV_INPUT_BUFFER_PADDING_SIZE，是为了防止某些优化过的Reader一次性读取过多导致越界
    char inDataArray[IN_DATA_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    char *inData = inDataArray;
    //文件
    QFile inFile(inFilename);
    QFile outFile(out.filename);
    //解码器
    AVCodec *codec = nullptr;
    //上下文
    AVCodecContext *ctx = nullptr;
    //解析器上下文
    AVCodecParserContext *parserCtx = nullptr;
    //存放解码前的数据
    AVPacket *pkt = nullptr;
    //存放解码后的数据
    AVFrame *frame = nullptr;
    //获取解码器
    codec = avcodec_find_decoder_by_name("libfdk_aac");
    if(!codec){
        qDebug() << "avcodec_find_decoder_by_name not found";
        return;
    }
    //初始化解析器上下文
    parserCtx = av_parser_init(codec->id);
    if(!parserCtx){
        qDebug() << "av_parser_init error";
        return;
    }
    //创建上下文
    ctx = avcodec_alloc_context3(codec);
    if(!ctx){
        qDebug() << "avcodec_alloc_context3 error";
        goto end;
    }
    qDebug() << "ctx初始化采样率:" << ctx->sample_rate;
    qDebug() << "ctx初始化采样格式:" << av_get_sample_fmt_name(ctx->sample_fmt) << ctx->sample_fmt;
    qDebug() << "ctx初始化声道数:" << av_get_channel_layout_nb_channels(ctx->channel_layout) << ctx->channel_layout;
    //创建AVPacket
    pkt = av_packet_alloc();
    //初始化后pkt的大小
    qDebug() << "av_packet_alloc后pkt大小:" << pkt->size;
    if(!pkt){
        qDebug() << "av_packet_alloc error";
        goto end;
    }
    //创建AVFrame
    frame = av_frame_alloc();
    if(!frame){
        qDebug() << "av_frame_alloc error";
        return;
    }
    //打开解码器
    ret = avcodec_open2(ctx,codec,nullptr);
    if(ret < 0){
       AV_ERROR(ret);
       qDebug() << "avcodec_open2 error" << errorbuf;
       goto end;
    }
    qDebug() << "ctx初始化打开解码器采样率:" << ctx->sample_rate;
    qDebug() << "ctx初始化打开解码器采样格式:" << av_get_sample_fmt_name(ctx->sample_fmt) << ctx->sample_fmt;
    qDebug() << "ctx初始化打开解码器声道数:" << av_get_channel_layout_nb_channels(ctx->channel_layout) << ctx->channel_layout;
    //打开文件
    if(!inFile.open(QFile::ReadOnly)){
        qDebug() << "file open error :" << inFile.fileName();
        goto end;
    }
    if(!outFile.open(QFile::WriteOnly)){
        qDebug() << "file open error :" << out.filename;
        goto end;
    }
    //解码
    //读取数据
    inLen = inFile.read(inData,IN_DATA_SIZE);
    while(inLen > 0){
        //初始化后pkt的大小
//        qDebug() << "传入解析器之前pkt大小:" << pkt->size;
        //经过解析器上下文处理
        ret = av_parser_parse2(parserCtx,ctx,&pkt->data,&pkt->size,(uint8_t *)inData,inLen,AV_NOPTS_VALUE,AV_NOPTS_VALUE,0);
        qDebug() << "ctx开始解析采样率:" << ctx->sample_rate;
        qDebug() << "ctx开始解析采样格式:" << av_get_sample_fmt_name(ctx->sample_fmt) << ctx->sample_fmt;
        qDebug() << "ctx开始解析声道数:" << av_get_channel_layout_nb_channels(ctx->channel_layout) << ctx->channel_layout;
        if(ret < 0){
            AV_ERROR(ret);
            qDebug() << "av_parser_parse2 error" << errorbuf;
            goto end;
        }
        inData += ret;
        inLen -= ret;
        //解码
        if(pkt->size <= 0) continue;
        if(decode(ctx,pkt,frame,outFile) < 0){
            goto end;
        }
        //如果数据不够了，再次读取数据
        if(inLen < REFILL_THRESH && !inEnd){
            //剩余数据移动到缓冲区前
            memmove(inDataArray,inData,inLen);
            inData = inDataArray;
            //跨过已有数据，读取文件
            int len = inFile.read(inData + inLen,IN_DATA_SIZE - inLen);
            if(len > 0){
                inLen += len;
            }else if(len == 0){
               inEnd = 1;
            }
        }
    }
    //刷新解码器
    //pkt->data = NULL;
    //pkt->size = 0;
    decode(ctx,nullptr,frame,outFile);
    //设置输出参数,若不设置采样率：1，采样格式：u8,声道数：14,但能正常播放
    //ctx打开关联解码器知道采样格式s16,开始解析后知道采样率44100和声道数2
    out.sampleFmt = ctx->sample_fmt;
    out.chLayout = ctx->channel_layout;
    out.sampleRate = ctx->sample_rate;
end:
    inFile.close();
    outFile.close();
    av_frame_free(&frame);
    av_packet_free(&pkt);
    av_parser_close(parserCtx);
    avcodec_free_context(&ctx);
}

