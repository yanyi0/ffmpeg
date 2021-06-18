#include "videoplayer.h"
#include <thread>
#pragma mark - 构造析构
VideoPlayer::VideoPlayer(QObject *parent) : QObject(parent)
{
    // 初始化Audio子系统
    if (SDL_Init(SDL_INIT_AUDIO)) {
        // 返回值不是0，就代表失败
        qDebug() << "SDL_Init Error" << SDL_GetError();
        emit playFailed(this);
        return;
    }
    _aPktList = new std::list<AVPacket>();
    _vPktList = new std::list<AVPacket>();
    _aMutex = new CondMutex();
    _vMutex = new CondMutex();
}
VideoPlayer::~VideoPlayer(){
    delete _aPktList;
    delete _vPktList;
    delete _aMutex;
    delete _vMutex;

    SDL_Quit();
}
#pragma mark - 公有方法
VideoPlayer::State VideoPlayer::getState(){
     return _state;
}
void VideoPlayer:: play(){
    if(_state == VideoPlayer::Playing) return;
    //状态可能是暂停，停止，播放完毕
    std::thread([this](){
        readFile();
    }).detach();
    //解封装，解码，播放，音视频同步
    //创建子线程去解码
    //解码后的格式不一定是我们播放器想要的
    //PCM格式不是SDL支持的 S16 44100
    //YUV RGB
    //改变状态
    setState(VideoPlayer::Playing);
}
void VideoPlayer::pause(){
    if(_state != VideoPlayer::Playing) return;
    //改变状态
    setState(VideoPlayer::Paused);
}

void VideoPlayer::stop(){
    if(_state == VideoPlayer::Stopped) return;
    //改变状态
    setState(VideoPlayer::Stopped);
}
bool VideoPlayer::isPlaying(){
    return _state == VideoPlayer::Playing;
}
void VideoPlayer::setFilename(const char *filename){
    _filename = filename;
}
int64_t VideoPlayer::getDuration(){
    return _fmtCtx ? _fmtCtx->duration:0;
}
#pragma mark - 私有方法
void VideoPlayer::readFile(){
//        qDebug() << "readFile线程" << QThread::currentThreadId() << "QT";
        //返回结果
        int ret = 0;
        //创建解封装上下文、打开文件
        ret = avformat_open_input(&_fmtCtx,_filename,nullptr,nullptr);
        END(avformat_open_input);
        //检索流信息
        ret = avformat_find_stream_info(_fmtCtx,nullptr);
        END(avformat_find_stream_info);
        //打印流信息到控制台
        av_dump_format(_fmtCtx,0,_filename,0);
        fflush(stderr);

        //初始化音频信息
        if(initAudioInfo() < 0)
            goto end;
       //初始化视频信息
        if(initVideoInfo() < 0)
            goto end;

        //初始化完毕，发送信号
        emit initFinished(this);
//        从输入文件中读取数据
        while(true){
            //确保每次读取到的pkt都是新的，若在while循环外面，则每次加入list中的pkt都将一模一样，为最后一次读取到的pkt
            AVPacket pkt;
            ret = av_read_frame(_fmtCtx,&pkt);
            if(ret == 0){
                if(pkt.stream_index == _aStream->index){//读取到的是音频数据
                    addAudioPkt(pkt);
                }else if(pkt.stream_index == _vStream->index){//读取到的是视频数据
                    addVideoPkt(pkt);
                }
            }else{
                continue;
            }
        }

    end:
        ;
        //后续统一进行释放
//        avcodec_free_context(&_aDecodeCtx);
//        avcodec_free_context(&_vDecodeCtx);
//        avformat_close_input(&_fmtCtx);
}
//初始化解码器:根据传入的AVMediaType获取解码信息，要想外面获取到解码上下文，传入外边的解码上下文的地址，同理传入stream的地址,里面赋值后
//外部能获取到，C语言中的指针改变传入的地址中的值
int VideoPlayer::initDecoder(AVCodecContext **decodeCtx,AVStream **stream,AVMediaType type){
    //根据TYPE寻找最合适的流信息
    //返回值是流索引
    int ret = av_find_best_stream(_fmtCtx,type,-1,-1,nullptr,0);
    RET(av_find_best_stream);
    //检验流
    int streamIdx = ret;
    qDebug() << "文件的流的数量" << _fmtCtx->nb_streams;
    *stream = _fmtCtx->streams[streamIdx];
    if(!stream){
        qDebug() << "stream is empty";
        return -1;
    }
    //为当前流找到合适的解码器
    AVCodec *decoder = nullptr;
    //音频解码器用libfdk_aac，不用ffmpeg默认自带的aac，默认自带的aac会解码成fltp格式的pcm，libfdk_aac会解码成sl6le的pcm
//    if(stream->codecpar->codec_id == AV_CODEC_ID_AAC){
//        decoder = avcodec_find_decoder_by_name("libfdk_aac");
//    }else{
        decoder = avcodec_find_decoder((*stream)->codecpar->codec_id);
//    }
    if(!decoder){
        qDebug() << "decoder not found" <<(*stream)->codecpar->codec_id;
        return -1;
    }
    //初始化解码上下文，打开解码器
    *decodeCtx = avcodec_alloc_context3(decoder);
    if(!decodeCtx){
        qDebug() << "avcodec_alloc_context3 error";
        return -1;
    }
    //从流中拷贝参数到解码上下文中
    ret = avcodec_parameters_to_context(*decodeCtx,(*stream)->codecpar);
    RET(avcodec_parameters_to_context);
    //打开解码器
    ret = avcodec_open2(*decodeCtx,decoder,nullptr);
    RET(avcodec_open2);
    return 0;
}


void VideoPlayer::setState(State state){
    if(state == _state) return;
    _state = state;
    emit stateChanged(this);
}
