#include "ffmpegs.h"
#include "QDebug"
//输入缓冲区的大小
#define IN_DATA_SIZE 4096
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
    //一帧图片的大小
    int imgSize = av_image_get_buffer_size(ctx->pix_fmt,ctx->width,ctx->height,1);
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
        //直接写入不正确，中间会有数据间隔
        //一帧图片中一行的y*高度=总共的Y的数量大小，一行的U*U的高度=总共U的数量大小，一行的V*V的高度
        outFile.write((char *)frame->data[0],frame->linesize[0]*ctx->height);
        outFile.write((char *)frame->data[1],frame->linesize[1]*ctx->height >> 1);
        outFile.write((char *)frame->data[2],frame->linesize[2]*ctx->height >> 1);
//        outFile.write((char *)frame->data[0],imgSize);
    }
}
void FFmpegs::h264Decode(const char *inFilename,VideoDecodeSpec &out){

    //返回结果
    int ret = 0;
    //每次从文件中读取的长度
    int inLen = 0;

    int inEnd = 0;
    //用来存放读取的文件数据
    char inDataArray[IN_DATA_SIZE];
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
//    codec = avcodec_find_decoder_by_name("h264");
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
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
    do{
        //读取数据
        inLen = inFile.read(inDataArray,IN_DATA_SIZE);
        inEnd = !inLen;
        //inData指向数组的首元素
        inData = inDataArray;
        //初始化后pkt的大小
//        qDebug() << "传入解析器之前pkt大小:" << pkt->size;
        while(inLen > 0 || inEnd){
            //经过解析器上下文处理
            ret = av_parser_parse2(parserCtx,ctx,&pkt->data,&pkt->size,(uint8_t *)inData,inLen,AV_NOPTS_VALUE,AV_NOPTS_VALUE,0);
            if(ret < 0){
                AV_ERROR(ret);
                qDebug() << "av_parser_parse2 error" << errorbuf;
                goto end;
            }
            inData += ret;
            inLen -= ret;
            qDebug() << inEnd << pkt->size << ret;
            //解码
            if(pkt->size <= 0) continue;
            if(decode(ctx,pkt,frame,outFile) < 0){
                goto end;
            }
            //若到了文件尾部，跳出循环
            if(inEnd) break;
        }
    }while(!inEnd);
    //刷新解码器
    //pkt->data = NULL;
    //pkt->size = 0;
    decode(ctx,nullptr,frame,outFile);
    out.width = ctx->width;
    out.height = ctx->height;
    out.pixFmt = ctx->pix_fmt;
    out.fps = ctx->framerate.num;
end:
    inFile.close();
    outFile.close();
    av_frame_free(&frame);
    av_packet_free(&pkt);
    av_parser_close(parserCtx);
    avcodec_free_context(&ctx);
}

