#include "yuvplayer.h"
#include <QDebug>
//出错了就执行goto end
#define RET(judge,func) \
    if(judge){ \
    qDebug() << #func << "Error" << SDL_GetError(); \
    return; \
}
extern "C" {
 #include <libavutil/imgutils.h>
}
#ifdef Q_OS_WIN32
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayVideo/in.bmp"
#else
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/RecordVideo/out3.yuv"
#endif
#define PIXEL_FORMAT SDL_PIXELFORMAT_UYVY
#define IMG_W 1280
#define IMG_H 720
static const std::map<AVPixelFormat,SDL_PixelFormatEnum> PIXEL_FORMAT_MAP = {
    { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
    { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
    { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
    { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
};
YuvPlayer::YuvPlayer(QWidget *parent) : QWidget(parent)
{
    //创建窗口
    _window = SDL_CreateWindowFrom((void *) winId());
    RET(!_window,SDL_CreateWindowFrom);
    //创建渲染上下文(默认的渲染目标是window)
    _renderer = SDL_CreateRenderer(_window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    //说明开启硬件加速失败
    if(!_renderer) _renderer = SDL_CreateRenderer(_window,-1,0);
    RET(!_renderer,SDL_CreateRenderer);
}
YuvPlayer::~YuvPlayer(){
    //释放资源
    _file.close();
    SDL_DestroyTexture(_texture);
    SDL_DestroyRenderer(_renderer);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}
YuvPlayer::State YuvPlayer::getState(){
     return _state;
}
void YuvPlayer:: play(){
    _timerId = startTimer(1000/_yuv.fps);
    _state = YuvPlayer::Playing;
}
void YuvPlayer::pause(){
    if(_timerId){
        killTimer(_timerId);
    }
    _state = YuvPlayer::Paused;
}
void YuvPlayer::stop(){
    if(_timerId){
        killTimer(_timerId);
    }
    _state = YuvPlayer::Stopped;
}
bool YuvPlayer::isPlaying(){
    return _state == YuvPlayer::Playing;
}
void YuvPlayer::setYuv(Yuv &yuv){
    _yuv = yuv;
    //创建纹理
    //PIXEL_FORMAT_MAP前面有const修饰，要加一个迭代器，通过second属性拿到，若没有const修饰PIXEL_FORMAT_MAP[yuv.pixelFmt]
    _texture = SDL_CreateTexture(_renderer,PIXEL_FORMAT_MAP.find(yuv.pixelFmt)->second,SDL_TEXTUREACCESS_STREAMING,yuv.width,yuv.height);
    RET(!_texture,SDL_CreateTexture);

    //打开yuv文件
    _file.setFileName(yuv.filename);
    if(!_file.open(QFile::ReadOnly)){
        qDebug() << "file open error" << yuv.filename;
        return;
    }
}
void YuvPlayer::timerEvent(QTimerEvent *event){
    int imgSize = av_image_get_buffer_size(_yuv.pixelFmt,_yuv.width,_yuv.height,1);
    char data[imgSize];
    if(_file.read(data,imgSize) > 0){
        //将YUV数据填充到texture
        RET(SDL_UpdateTexture(_texture,nullptr,data,_yuv.width),SDL_UpdateTexture);
        //设置红色的画笔颜色
        RET(SDL_SetRenderDrawColor(_renderer,0,0,0,SDL_ALPHA_OPAQUE),SDL_SetRenderDrawColor);
    //    //用绘制颜色(画笔颜色)DrawColor清除渲染目标
        RET(SDL_RenderClear(_renderer),SDL_RenderClear);
        //复制纹理数据到渲染目标(默认是window)上,从srcRect的大小,等比例复制到dstRect上,传NULL,表示全部的纹理大小渲染到全部的渲染目标上
        RET(SDL_RenderCopy(_renderer,_texture,nullptr,nullptr),SDL_RenderCopy);
        //将此前所有需要渲染的内容更新到屏幕上
        SDL_RenderPresent(_renderer);
    }else{//文件数据已经读取完毕
        if(_timerId){
            killTimer(_timerId);
        }
    }
}
