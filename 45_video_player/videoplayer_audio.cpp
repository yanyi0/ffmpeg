#include "videoplayer.h"
/* 一些宏定义 */
// 采样率
#define SAMPLE_RATE 44100
// 采样格式
#define SAMPLE_FORMAT AUDIO_S16LSB
// 采样大小
#define SAMPLE_SIZE SDL_AUDIO_BITSIZE(SAMPLE_FORMAT)
// 声道数
#define CHANNELS 2
// 音频缓冲区的样本数量
#define SAMPLES 512
//初始化音频信息
int VideoPlayer::initAudioInfo(){
    /*
     * 命令行生成yuv和pcm  ffmpeg自带音频解码器默认输出fltp格式pcm，libfdk_aac默认输出s16le
       ffmpeg -c:v h264 -c:a libfdk_aac -i in.mp4 cmd_out.yuv -f s16le cmd_out.pcm
       ffmpeg自带解码器输出pcm
       ffmpeg -c:a aac -i in.mp4  -f s16le cmd_out_aac.pcm
     */
    //根据TYPE寻找最合适的流信息
    //初始化解码器
    int ret = initDecoder(&_aDecodeCtx,&_aStream,AVMEDIA_TYPE_AUDIO);
    RET(initDecoder);

    //初始化_aFrame
    _aFrame = av_frame_alloc();
    if(!_aFrame){
        qDebug() << "av_frame_alloc error";
        return -1;
    }

    //初始化SDL
    ret = initSDL();
    RET(initSDL);

    return 0;
}
int VideoPlayer::initSDL(){
    // 音频参数
    SDL_AudioSpec spec;
    // 采样率
    spec.freq = SAMPLE_RATE;
    // 采样格式（s16le）
    spec.format = SAMPLE_FORMAT;
    // 声道数
    spec.channels = CHANNELS;
    // 音频缓冲区的样本数量（这个值必须是2的幂）
    spec.samples = SAMPLES;
    // 回调
    spec.callback = sdlAudioCallbackFunc;
    // 传递给回调的参数
    spec.userdata = this;
    // 打开音频设备
    if (SDL_OpenAudio(&spec, nullptr)) {
        qDebug() << "SDL_OpenAudio Error" << SDL_GetError();
        return -1;
    }
    //开始播放pcm
    SDL_PauseAudio(0);
    return 0;
}
void VideoPlayer::sdlAudioCallbackFunc(void *userdata, Uint8 *stream, int len){
    VideoPlayer *player = (VideoPlayer *)userdata;
    player->sdlAudioCallback(stream,len);
}
void VideoPlayer::addAudioPkt(AVPacket &pkt){
    _aMutex->lock();
    _aPktList->push_back(pkt);
    _aMutex->signal();
    _aMutex->unlock();
}
void VideoPlayer::clearAudioPktList(){
    _aMutex->lock();
    //取出list，前面加*
    for(AVPacket &pkt:*_aPktList){
        av_packet_unref(&pkt);
    }
    _aPktList->clear();
    _aMutex->unlock();
}
void VideoPlayer::sdlAudioCallback(Uint8 *stream, int len){
//    QString LogInfo;
//    LogInfo.sprintf("%p", QThread::currentThread());
//    qDebug() << "-----sdlAudioCallback---------OpenSerialPort " <<"threadID : "<<LogInfo;
   //len音频缓冲区剩余的大小(音频缓冲区剩余要填充的大小)
    while (len > 0) {
        int dataSize = decodeAudio();
        qDebug() << "解码一次的数据大小" << dataSize;
        if(dataSize <= 0){

        }else{

        }
        //将一个pkt包解码后的pcm数据填充到SDL的音频缓冲区
//        SDL_MixAudio(stream,src,srcLen,SDL_MIX_MAXVOLUME);
//        //移动偏移量
//        len -= srcLen;
//        stream -= srcLen;
    }
}
int VideoPlayer::decodeAudio(){
//    qDebug() << "decodeAudio线程" << QThread::currentThreadId() << "QT";
    _aMutex->lock();
    //_aPktList中如果是空的，就进入等待，等待_aPktList中新加入解封装后的pkt发送信号signal通知到这儿，
    //有可能解封装很快就都解完成了，后面都没有新的pkt，也不会发送信号了,就会一直在这儿等
//    while(_aPktList->empty()){
//        _aMutex->wait();
//    }
    if(_aPktList->empty()){
        _aMutex->unlock();
        return 0;
    }
    //取出list中的头部pkt
    AVPacket pkt = _aPktList->front();
    //删除头部pkt
    _aPktList->pop_front();
    _aMutex->unlock();
    // 发送压缩数据到解码器
    int ret = avcodec_send_packet(_aDecodeCtx, &pkt);
    //释放pkt
    av_packet_unref(&pkt);
    RET(avcodec_send_packet);
    // 获取解码后的数据
    ret = avcodec_receive_frame(_aDecodeCtx, _aFrame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return 0;
    } else RET(avcodec_receive_frame);
    qDebug() << _aFrame->sample_rate << _aFrame->channels << av_get_sample_fmt_name((AVSampleFormat)_aFrame->format);
    //由于解码出来的pcm和SDL要求的pcm格式可能不一致，需要进行音频重采样
    return _aFrame->sample_rate * _aFrame->channels * av_get_bytes_per_sample((AVSampleFormat)_aFrame->format);
}
