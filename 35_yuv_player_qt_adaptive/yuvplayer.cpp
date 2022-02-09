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
    //设置黑色背景
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
    //适配播放器
    /*
     if(图片宽度<=播放器宽度 && 图片高度<=播放器高度){
         //居中 画面嵌套在播放器中间
     }else{
          if(图片宽度>播放器宽度 && 图片高度> 播放器高度){
          }
          else if(图片宽度>播放器宽度){
          }else if(图片高度>播放器高度){
          }
      }
     */
    //组件的尺寸
    int w = width();
    int h = height();
    //计算rect
    int dx = 0;
    int dy = 0;
    int dw = _yuv.width;
    int dh = _yuv.height;
    //计算目标尺寸
    //图片的宽高比大于播放器的宽高比，则缩放图片到图片的宽度等于播放器的宽度
    //图片的高宽比大于播放器的高宽比，则缩放图片到图片的高度等于播放器的高度
    /*
    if(dw/dh > w/h){
        dw = w;
        dh = 计算出来
    }else{
        dh = h;
        dw = 计算出来
    }
     */
    if(dw > w || dh > h){ //图片的宽度大于播放器宽度或图片的高度大于播放器的高度都要进行缩放
        if(dw * h > w * dh){//视频的宽高比 > 播放器的宽高比   视频的宽度=播放器的宽度 等比例压缩求出缩放后的视频的高度
            dh = dh * w/dw;
            dw = w;
        }else{//视频的高宽比 > 播放器的高宽比 视频的高度= 播放器的高度  等比例压缩求出缩放后的视频的宽度
            dw = dw * h/dh;
            dh = h;
        }
    }
    //居中
    dx = (w-dw) >> 1;
    dy = (h-dh) >> 1;

    qDebug() << "缩放后的视频尺寸矩形框" << dx << dy << dw << dh;
    _dstRect = QRect(dx,dy,dw,dh);
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
    //将图片渲染到整个播放器上面:明显改变了宽高比，发现画面变形了
//    painter.drawImage(QRect(0,0,width(),height()),*_currentImage);
    painter.drawImage(_dstRect,*_currentImage);
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
