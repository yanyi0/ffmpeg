#include "demuxer.h"
#include "QDebug"
#define ERROR_BUF \
    char errbuf[1024];\
    av_strerror(ret,errbuf,sizeof(errbuf));
#define END(func) \
    if(ret < 0) { \
    ERROR_BUF;\
    qDebug() << #func << "error" << errbuf;\
    goto end;\
    }

#define RET(func) \
    if(ret < 0) { \
    ERROR_BUF;\
    qDebug() << #func << "error" << errbuf;\
    return ret;\
    }
Demuxer::Demuxer()
{

}
void Demuxer::demux(const char *inFilename,AudioDecodeSpec &aOut,VideoDecodeSpec &vOut){
    //指针变量_aOut,_vOut指向原始传入的地址,最后会改变外部传入时的值
    _aOut = &aOut;
    _vOut = &vOut;
    //返回结果
    int ret = 0;
    //存放解码前的数据
    AVPacket *pkt;
    //创建解封装上下文、打开文件
    ret = avformat_open_input(&_fmtCtx,inFilename,nullptr,nullptr);
    END(avformat_open_input);
    //检索流信息
    ret = avformat_find_stream_info(_fmtCtx,nullptr);
    END(avformat_find_stream_info);
    //打印流信息到控制台
    av_dump_format(_fmtCtx,0,inFilename,0);
    fflush(stderr);

    //初始化音频信息
    ret = initAudioInfo();
    if(ret < 0)
        goto end;
   //初始化视频信息
    ret = initVideoInfo();
    if(ret < 0)
        goto end;

    //初始化frame
    _frame = av_frame_alloc();
    if(!_frame){
        qDebug() << "av_frame_alloc error";
        goto end;
    }

    //初始化pkt:4.4版本后被摧毁，用av_packet_alloc()替代
    //定义为局部变量AVPacket *pkt指针变量,指针变量的属性用箭头，对象用.
    pkt = av_packet_alloc();
//    av_init_packet(&pkt);定义成AVPacket pkt;开辟栈空间
    pkt->data = nullptr;
    pkt->size = 0;

    //从输入文件中读取数据
    while(av_read_frame(_fmtCtx,pkt) == 0){
        qDebug() << "当前读取到的流索引:" << pkt->stream_index;
        if(pkt->stream_index == _aStreamIdx){//读取到的是音频数据
            ret = decode(_aDecodeCtx,pkt);
        }else if(pkt->stream_index == _vStreamIdx){//读取到的是视频数据
            ret = decode(_vDecodeCtx,pkt);
        }
        //并不是释放pkt的内存，pkt还在，释放掉pkt内部指针指向的一些额外的内存
        av_packet_unref(pkt);
        if(ret < 0)
            goto end;
    }
    //刷新缓冲区
    decode(_aDecodeCtx,nullptr);
    decode(_vDecodeCtx,nullptr);
end:
    _aOutFile.close();
    _vOutFile.close();
    avcodec_free_context(&_aDecodeCtx);
    avcodec_free_context(&_vDecodeCtx);
    avformat_close_input(&_fmtCtx);
    av_frame_free(&_frame);
    av_packet_free(&pkt);
}
//初始化解码器:根据传入的AVMediaType获取解码信息，要想外面获取到解码上下文，传入外边的解码上下文的地址，同理传入stream的地址,里面赋值后
//外部能获取到，C语言中的指针改变传入的地址中的值
int Demuxer::initDecoder(AVCodecContext **decodeCtx,int *streamIdx,AVMediaType type){
    //根据TYPE寻找最合适的流信息
    //返回值是流索引
    int ret = av_find_best_stream(_fmtCtx,type,-1,-1,nullptr,0);
    RET(av_find_best_stream);
    //检验流
    *streamIdx = ret;
    qDebug() << "文件的流的数量" << _fmtCtx->nb_streams;
    AVStream *stream = _fmtCtx->streams[*streamIdx];
    if(!stream){
        qDebug() << "stream is empty";
        return -1;
    }
    //为当前流找到合适的解码器
    AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if(!decoder){
        qDebug() << "decoder not found" << stream->codecpar->codec_id;
        return -1;
    }
    //初始化解码上下文，打开解码器
    *decodeCtx = avcodec_alloc_context3(decoder);
    if(!decodeCtx){
        qDebug() << "avcodec_alloc_context3 error";
        return -1;
    }
    //从流中拷贝参数到解码上下文中
    ret = avcodec_parameters_to_context(*decodeCtx,stream->codecpar);
    RET(avcodec_parameters_to_context);
    //打开解码器
    ret = avcodec_open2(*decodeCtx,decoder,nullptr);
    RET(avcodec_open2);
    return 0;
}
//初始化音频信息
int Demuxer::initAudioInfo(){
    /*
     * 命令行生成yuv和pcm  ffmpeg自带音频解码器默认输出fltp格式pcm，libfdk_aac默认输出s16le
       ffmpeg -c:v h264 -c:a libfdk_aac -i in.mp4 cmd_out.yuv -f s16le cmd_out.pcm
       ffmpeg自带解码器输出pcm
       ffmpeg -c:a aac -i in.mp4  -f s16le cmd_out_aac.pcm
     */
    //根据TYPE寻找最合适的流信息
    //初始化解码器
    int ret = initDecoder(&_aDecodeCtx,&_aStreamIdx,AVMEDIA_TYPE_AUDIO);
    qDebug() << "初始化音频信息后查看音频流索引" << _aStreamIdx;
    if(ret < 0) return  ret;
    //打开文件
    _aOutFile.setFileName(_aOut->filename);
    if(!_aOutFile.open(QFile::WriteOnly)){
       qDebug() << "file open error" << _aOut->filename;
       return -1;
    }
    //保存音频参数
    _aOut->sampleFmt = _aDecodeCtx->sample_fmt;
    _aOut->sampleRate = _aDecodeCtx->sample_rate;
    _aOut->chLayout = _aDecodeCtx->channel_layout;
    return 0;
}
//初始化视频信息
int Demuxer::initVideoInfo(){
    int ret = initDecoder(&_vDecodeCtx,&_vStreamIdx,AVMEDIA_TYPE_VIDEO);
    qDebug() << "初始化视频信息后查看视频流索引" << _vStreamIdx;
    if(ret < 0) return  ret;
    //打开文件
    _vOutFile.setFileName(_vOut->filename);
    if(!_vOutFile.open(QFile::WriteOnly)){
       qDebug() << "file open error" << _vOut->filename;
       return -1;
    }
    //保存视频参数
    _vOut->pixFmt = _vDecodeCtx->pix_fmt;
    _vOut->width = _vDecodeCtx->width;
    _vOut->height = _vDecodeCtx->height;
    AVRational framerate = av_guess_frame_rate(_fmtCtx,_fmtCtx->streams[_vStreamIdx],nullptr);
    _vOut->fps = framerate.num/framerate.den;
    return 0;
}
int Demuxer::decode(AVCodecContext *decodeCtx,AVPacket *pkt){
    //发送压缩数据到解码器
    int ret = avcodec_send_packet(decodeCtx,pkt);
    RET(avcodec_send_packet);
    while(true){
        //获取解码后的数据
        ret = avcodec_receive_frame(decodeCtx,_frame);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            qDebug() << "进入文件尾部";
            return 0;
        }
        RET(avcodec_receive_frame)
        //将解码后的数据frame写入文件·
        if(decodeCtx->codec->type == AVMEDIA_TYPE_VIDEO){
//        if(pkt->stream_index == _aStreamIdx){
            //可以多种选择方式判断,这儿用流索引判断，刷新缓冲区的时候pkt为null，会导致程序空指针崩溃，无法用流索引进行判断是否为音频还是视频文件
           writeVideoFrame();
        }else{
           writeAudioFrame();
        }
    }
}
void Demuxer::writeAudioFrame(){

}

void Demuxer::writeVideoFrame(){
    //直接写入不正确，中间会有数据间隔
    //一帧图片中一行的y*高度=总共的Y的数量大小，一行的U*U的高度=总共U的数量大小，一行的V*V的高度
    _vOutFile.write((char *)_frame->data[0],_frame->linesize[0]*_vOut->height);
    _vOutFile.write((char *)_frame->data[1],_frame->linesize[1]*_vOut->height >> 1);
    _vOutFile.write((char *)_frame->data[2],_frame->linesize[2]*_vOut->height >> 1);
};
