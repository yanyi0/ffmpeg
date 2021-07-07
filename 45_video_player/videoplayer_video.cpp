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

    //开启新的线程去解码视频数据
    std::thread([this](){
        decodeVideo();
    }).detach();
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

    //初始化像素格式转换上下文
    _vSwsCtx = sws_getContext(inW,
                              inH,
                              _vDecodeCtx->pix_fmt,
                              _vSwsOutSpec.width,
                              _vSwsOutSpec.height,
                              _vSwsOutSpec.pixFmt,
                              SWS_BILINEAR,nullptr,nullptr,nullptr);

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
    clearVideoPktList();
    avcodec_free_context(&_vDecodeCtx);
    av_frame_free(&_vSwsInFrame);
    if(_vSwsOutFrame){
        av_freep(&_vSwsOutFrame->data[0]);
        av_frame_free(&_vSwsOutFrame);
    }
    sws_freeContext(_vSwsCtx);
    _vSwsCtx = nullptr;
}
void VideoPlayer::decodeVideo(){
    while (true) {
        //如果是停止状态，会调用free，就不用再去解码，重采样，渲染，导致访问释放了的内存空间，会闪退
        if(_state == Stopped) break;
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
        //释放pkt
        av_packet_unref(&pkt);
        CONTINUE(avcodec_send_packet);
        while (true) {
            //获取解码后的数据
            ret = avcodec_receive_frame(_vDecodeCtx,_vSwsInFrame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                break;//结束本次循环，重新从_vPktList取出包进行解码
            }else BREAK(avcodec_receive_frame);

            //TODO 假停顿
            // 1000 / 30
            SDL_Delay(33);
            //像素格式转换
            //_vSwsInFrame(yuv420p) -> _vSwsOutFrame(rgb24)
            sws_scale(_vSwsCtx,_vSwsInFrame->data,_vSwsInFrame->linesize,0,_vSwsInFrame->height,_vSwsOutFrame->data,_vSwsOutFrame->linesize);

//            qDebug() << _vSwsOutFrame->data[0];
            //发出信号
            emit frameDecoded(this,_vSwsOutFrame->data[0],_vSwsOutSpec);
        }
    }
}
