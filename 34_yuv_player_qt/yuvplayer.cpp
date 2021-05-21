#include "yuvplayer.h"
#include <QDebug>
#include <QPainter>
#include "ffmpegs.h"
//出错了就执行goto end
#define RET(judge,func) \
    if(judge){ \
    qDebug() << #func << "Error" << SDL_GetError(); \
    return; \
}
extern "C" {
 #include <libavutil/imgutils.h>
}
YuvPlayer::YuvPlayer(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground,true);
    setStyleSheet("background:black");
}
YuvPlayer::~YuvPlayer(){
    _file.close();
    //视频的最后一帧图片释放
    freeCurrentImage();
}
YuvPlayer::State YuvPlayer::getState(){
     return _state;
}
void YuvPlayer:: play(){
    _timerId = startTimer(1000/_yuv.fps);
    _state = YuvPlayer::Playing;
    qDebug() << "开启定时器";
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
    //打开文件
    _file.setFileName(yuv.filename);
    if(!_file.open(QFile::ReadOnly)){
        qDebug() << "file open error" << _file.fileName();
    }
}
//当组件想重绘的时候会调用这个函数
//想要绘制什么内容，在这个函数中实现
void YuvPlayer::paintEvent(QPaintEvent *event){
    //第一次还没有图片直接返回
    if(!_currentImage) return;
    QPainter painter(this);
    //将图片绘制到当前组件上
    //从矩形框的左上角开始绘制一个帧的宽高
//    painter.drawImage(QPoint(0,0),*_currentImage);
    painter.drawImage(QRect(0,0,width(),height()),*_currentImage);
}
void YuvPlayer::timerEvent(QTimerEvent *event){
    //33.33333333毫秒进入
    int imgSize = av_image_get_buffer_size(_yuv.pixelFmt,_yuv.width,_yuv.height,1);
    char data[imgSize];
    if(_file.read(data,imgSize) > 0){
        RawVideoFrame in = {
            data,
            _yuv.width,_yuv.height,
            _yuv.pixelFmt,
        };
        RawVideoFrame out = {
            nullptr,
            _yuv.width,_yuv.height,
            AV_PIX_FMT_RGB24,
        };
        FFmpegs::convertVideoFrame(in,out);
        //渲染下一帧图片前释放上一帧图片
        freeCurrentImage();
        //保存转化格式后的这一帧图片
        _currentImage = new QImage((uchar *)out.pixels,out.width,out.height,QImage::Format_RGB888);
        //更新UI:触发void YuvPlayer::paintEvent(QPaintEvent *event)渲染这一帧图片
        update();
    }else{//文件数据已经读取完毕
        if(_timerId){
            killTimer(_timerId);
        }
    }
}
//渲染一帧图片，释放一帧图片
void YuvPlayer::freeCurrentImage(){
    if(!_currentImage) return;
    free(_currentImage->bits());
    delete _currentImage;
    _currentImage = nullptr;
}
