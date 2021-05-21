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
    //关掉文件
    closeFile();
    //视频的最后一帧图片释放
    freeCurrentImage();
    //关闭定时器
    stopTimer();
}
YuvPlayer::State YuvPlayer::getState(){
     return _state;
}
void YuvPlayer::setState(State state){
    if(state == _state) return;
    if(state == YuvPlayer::Stopped || state == YuvPlayer::YuvPlayer::Finished){
        //让文件读取指针回到文件首部，方便下次播放时从头开始播放
        _file->seek(0);
    }
    _state = state;
    emit stateChanged();
}
void YuvPlayer:: play(){
    if(_state == YuvPlayer::Playing) return;
    //状态可能是:暂停，停止，播放完毕，才能往下走
    //如果外部连续调用play()方法，则会导致开启多个定时器
    _timerId = startTimer(_interval);
    //改变状态
    setState(YuvPlayer::Playing);
    qDebug() << "开启定时器";
}
void YuvPlayer::pause(){
    if(_state != YuvPlayer::Playing) return;
    //状态只可能是正在播放状态，才能暂停，其他暂停，停止，播放完毕都不能暂停了
    stopTimer();
    //改变状态
    setState(YuvPlayer::Paused);
}
/*
 * 报错
 * timer id 3 is not valid for object 0x2fb6930(YuvPlayer),timer has not been killed
 * QObject::killTimer(): Error: timer id 2 is not valid for object 0x7f9e8182cd70 (YuvPlayer, ), timer has not been killed
 * 正常播放完毕后killTimer(3),此时_timerId = 3,此时点击停止，又执行killTimer(3)将3kill一遍，定时器3已经不存在了，所以此时报错
 */
void YuvPlayer::stop(){
    if(_state == YuvPlayer::Stopped) return;
    //状态可能是:正在播放，暂停，播放完毕
    stopTimer();
    //播放停止:释放图片,画面变黑
    freeCurrentImage();
    //刷新帧，使当前画面变黑
    update();
    //要调用repaint()画面才会黑掉
    repaint();
    //改变状态
    setState(YuvPlayer::Stopped);
}
bool YuvPlayer::isPlaying(){
    return _state == YuvPlayer::Playing;
}
void YuvPlayer::setYuv(Yuv &yuv){
    _yuv = yuv;
    //关掉上一个文件
    closeFile();
    //一帧图片的大小
    _imgSize = av_image_get_buffer_size(_yuv.pixelFmt,_yuv.width,_yuv.height,1);
    //刷帧的时间间隔
    _interval = 1000/_yuv.fps;
    //打开文件
    _file = new QFile(yuv.filename);
    if(!_file->open(QFile::ReadOnly)){
        qDebug() << "file open error" << yuv.filename;
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
    char data[_imgSize];
    //读取到的是满一帧数据才播放
    if(_file->read(data,_imgSize) == _imgSize){
        RawVideoFrame in = {
            data,
            _yuv.width,_yuv.height,
            _yuv.pixelFmt,
        };
        //有个细节需要注意:两种pixel_format相互转换时，要宽度和高度是16的倍数，不符合16的倍数可能会转化速度降低
        //报警告:Warning:data is not aligned! this can lead to a speed loss
        //_yuv.width >> 4 << 4,找一个比较接近_yuv.width又是16的倍数的，先右移4位除以16，再左移4位乘以16，
        //比如888/16*16,找到了一个接近888的整数是16的倍数,整数乘以整数会丢失掉一些小数部位
        //输入数据不是16的倍数报警告，是无法改变的，改变后读取数据会出错，就乱了
        RawVideoFrame out = {
            nullptr,
            _yuv.width >> 4 << 4,_yuv.height >> 4 << 4,
            AV_PIX_FMT_RGB24,
        };
        FFmpegs::convertVideoFrame(in,out);
        //渲染下一帧图片前释放上一帧图片
        freeCurrentImage();
        //保存转化格式后的这一帧图片
        _currentImage = new QImage((uchar *)out.pixels,out.width,out.height,QImage::Format_RGB888);
        //更新UI:触发void YuvPlayer::paintEvent(QPaintEvent *event)渲染这一帧图片
        update();
    }else{
        //文件数据已经读取完毕
        //停止定时器
        stopTimer();
        //正常播放完毕
        setState(YuvPlayer::Finished);
    }
}
//渲染一帧图片，释放一帧图片
void YuvPlayer::freeCurrentImage(){
    if(!_currentImage) return;
    free(_currentImage->bits());
    delete _currentImage;
    _currentImage = nullptr;
}
//释放定时器
void YuvPlayer::stopTimer(){
    if(_timerId == 0) return;
    killTimer(_timerId);
    _timerId = 0;
}
//关闭文件
void YuvPlayer::closeFile(){
     if(!_file) return;
     //关掉文件流
     _file->close();
     //删掉内存空间
     delete _file;
     //指针指向nullptr
     _file = nullptr;
}
