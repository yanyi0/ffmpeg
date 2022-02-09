#include "videoplayer.h"
extern "C" {
 #include <libavutil/imgutils.h>
}
//初始化视频信息
int VideoPlayer::initVideoInfo(){
    int ret = initDecoder(&_vDecodeCtx,&_vStream,AVMEDIA_TYPE_VIDEO);
    RET(initDecoder);

    //初始化视频像素格式转换
    ret = initSws();
    RET(initSws);

    return 0;
}
//初始化视频像素格式转换
int VideoPlayer::initSws(){
    int inW = _vDecodeCtx->width;
    int inH = _vDecodeCtx->height;
    //输出frame的参数 输出的图片宽高必须是16的倍数
    _vSwsOutSpec.width = inW >> 4 << 4;
    _vSwsOutSpec.height = inH >> 4 << 4;
    _vSwsOutSpec.pixFmt = AV_PIX_FMT_RGB24;
    _vSwsOutSpec.size = av_image_get_buffer_size(_vSwsOutSpec.pixFmt,_vSwsOutSpec.width,_vSwsOutSpec.height,1);

    //初始化像素格式转换上下文
    _vSwsCtx = sws_getContext(inW,
                              inH,
                              _vDecodeCtx->pix_fmt,
                              _vSwsOutSpec.width,
                              _vSwsOutSpec.height,
                              _vSwsOutSpec.pixFmt,
                              SWS_BILINEAR,nullptr,nullptr,nullptr);
    if(!_vSwsCtx){
        qDebug() << "sws_getContext error";
        return -1;
    }

    //初始化输入Frame
    _vSwsInFrame = av_frame_alloc();
    if(!_vSwsInFrame){
        qDebug() << "av_frame_alloc error";
        return -1;
    }

    //初始化输出Frame
    _vSwsOutFrame = av_frame_alloc();
    if(!_vSwsOutFrame){
        qDebug() << "av_frame_alloc error";
        return -1;
    }
    //初始化__vSwsOutFrame的data[0]指向的内存空间
    //初始化_vSwsOutFrame后，并没有给_vSwsOutFrame中的data[0],data[1],data[2]...分配指向的内存空间，
    //像素格式转化后，需要存放转换好的RGB数据，需要事先分配好存放的内存空间,否则转换像素格式后的_vSwsOutFrame->data数据为0x000
    //_vSwsInFrame不需要创建data[0] data[1] data[2]指向的内存空间，因为avcodec_receive_frame方法里面帮忙创建了data[0] data[1] data[2]内存空间，并用完后释放
    //最后一次会内存泄露吗? 调用avcodec_receive_frame必定是AVERROR(EAGAIN)或AVERROR_EOF或解码错误，说明没有拿到解码数据，data[0] data[1] data[2]均为空，
    //不会创建内存空间，没有申请到内存空间，所以不用释放，故不会内存泄露
    int ret = av_image_alloc(_vSwsOutFrame->data,_vSwsOutFrame->linesize,_vSwsOutSpec.width,_vSwsOutSpec.height,_vSwsOutSpec.pixFmt,1);
    RET(av_image_alloc);

    return 0;
}
void VideoPlayer::addVideoPkt(AVPacket &pkt){
    _vMutex.lock();
    _vPktList.push_back(pkt);
    _vMutex.signal();
    _vMutex.unlock();
}
void VideoPlayer::clearVideoPktList(){
    _vMutex.lock();
    //取出list，前面加*
    for(AVPacket &pkt:_vPktList){
        av_packet_unref(&pkt);
    }
    _vPktList.clear();
    _vMutex.unlock();
}
void VideoPlayer::freeVideo(){
    _vTime = 0;
    _vCanFree = false;
    clearVideoPktList();
    avcodec_free_context(&_vDecodeCtx);
    av_frame_free(&_vSwsInFrame);
    if(_vSwsOutFrame){
        av_freep(&_vSwsOutFrame->data[0]);
        av_frame_free(&_vSwsOutFrame);
    }
    sws_freeContext(_vSwsCtx);
    _vSwsCtx = nullptr;
    _vStream = nullptr;
    _vSeekTime = -1;
}
void VideoPlayer::decodeVideo(){
//    qDebug() << "当前的播放器状态--------" << this->getState();
    while (true) {
        //如果是暂停状态,并且没有seek操作，暂停状态也能seek
        if(_state == Paused && _vSeekTime == -1) continue;
        //如果是停止状态，会调用free，就不用再去解码，重采样，渲染，导致访问释放了的内存空间，会闪退
        if(_state == Stopped){
            _vCanFree = true;
            break;
        }
//        qDebug() << "------------正式开始解码视频数据了-----------";
        _vMutex.lock();
        if(_vPktList.empty()){
            _vMutex.unlock();
            continue;
        }
        //取出头部的视频包
        AVPacket pkt = _vPktList.front();
        _vPktList.pop_front();
        _vMutex.unlock();

        //发送数据到解码器
        int ret = avcodec_send_packet(_vDecodeCtx,&pkt);

        //视频时钟 视频用dts，音频用pts
        if(pkt.dts != AV_NOPTS_VALUE){
//            qDebug() << _vStream->time_base.num << _vStream->time_base.den;
            _vTime = av_q2d(_vStream->time_base) * pkt.dts;
            qDebug() << "当前视频时间"<< _vTime << "seek时间" << _vSeekTime;
        }

        //释放pkt
        av_packet_unref(&pkt);
        CONTINUE(avcodec_send_packet);
        while (true) {
            //获取解码后的数据
            ret = avcodec_receive_frame(_vDecodeCtx,_vSwsInFrame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;//结束本次循环，重新从_vPktList取出包进行解码
            }else BREAK(avcodec_receive_frame);

            //一定要在解码成功后，再进行下面的判断,防止seek时，到达的是p帧，但前面的I帧已经被释放了，无法参考，这一帧的解码就会出现撕裂现象
            //发现视频的时间是早于seekTime的，就丢弃，防止到seekTime的位置闪现
            if(_vSeekTime >= 0){
                if(_vTime < _vSeekTime){
                    continue;
                }else{
                    _vSeekTime = -1;
                }
            }

            //像素格式转换
            //_vSwsInFrame(yuv420p) -> _vSwsOutFrame(rgb24)
            sws_scale(_vSwsCtx,_vSwsInFrame->data,_vSwsInFrame->linesize,0,_vSwsInFrame->height,_vSwsOutFrame->data,_vSwsOutFrame->linesize);

            if(_hasAudio){//有音频
                //如果视频包多早解码出来，就要等待对应的音频时钟到达
                //有可能点击停止的时候，正在循环里面，停止后sdl free掉了，就不会再从音频list中取出包，_aClock就不会增大，下面while就死循环了，一直出不来，所以加Playing判断
                while(_vTime > _aTime && _state == Playing){
//                    SDL_Delay(5);
//                    qDebug() << "当前的音视频时钟------" << _vTime << _aTime;
                }
            }else{
                //TODO 没有音频的情况
            }

//            qDebug() << _vSwsOutFrame->data[0];
            //子线程把这一块数据_vSwsOutFrame->data[0]直接发送到主线程，给widget里面的image里面的bits指针指向去绘制图片，主线程也会访问这一块内存数据，子线程也会访问，有可能子线程正往里面写着，主线程就拿去用了，会导致数据错乱，崩溃
            //将像素转换后的图片数据拷贝一份出来
            uint8_t *data = (uint8_t *)av_malloc(_vSwsOutSpec.size);
//            uint8_t *data = new uint8_t[_vSwsOutSpec.size];
            memcpy(data,_vSwsOutFrame->data[0],_vSwsOutSpec.size);
            //发出信号
            emit frameDecoded(this,data,_vSwsOutSpec);
            qDebug() << "渲染了一帧" << _vTime << "音频时间" << _aTime;
        }
    }
}
