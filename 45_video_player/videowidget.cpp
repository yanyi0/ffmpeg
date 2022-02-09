#include "videowidget.h"
#include <QDebug>
#include <QPainter>
// VideoWidget:显示渲染视频数据
// VideoPlayer:预处理视频数据(解析文件、解码视频流、音频流)
VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent)
{
    //设置黑色背景
    setAttribute(Qt::WA_StyledBackground,true);
    setStyleSheet("background:black");
   qDebug() << "VideoWidget::VideoWidget";
}
VideoWidget::~VideoWidget(){
    freeImage();
}
void VideoWidget::paintEvent(QPaintEvent *event){
//    qDebug() << "-----木前有图片嘛-------";
    //第一次还没有图片直接返回
    if(!_image) return;
//    qDebug() << "-----渲染图片嘛-------";
    //将图片绘制到当前组件上
    QPainter(this).drawImage(_rect,*_image);
}
void VideoWidget::onPlayerStateChanged(VideoPlayer *player){
     if(player->getState() != VideoPlayer::Stopped) return;
     qDebug() << "----------VideoWidget::onPlayerStateChanged------------";
     freeImage();
     //黑屏 图片释放掉之后变黑屏
     update();
     qDebug() << "VideoWidget::onPlayerStateChanged" << _image;

}
void VideoWidget::onPlayerFrameDecoded(VideoPlayer *player,uint8_t *data,VideoPlayer::VideoSwsSpec &spec){
    //有可能点击了停止按钮后，还在继续解码下一帧，先收到onPlayerStateChanged，再onPlayerFrameDecoded，播放下一个视频的时候，会闪现前一个视频的最后一帧
    //杜绝最后一帧图片还显示为上一个视频的图片
     if(player->getState() == VideoPlayer::Stopped) return;
//    qDebug() << "----------已经解码好了图片-------------";
    //释放之前的图片
    freeImage();
//    qDebug() << "VideoWidget::onPlayerFrameDecoded" << _image;
    //创建新的图片
    if(data != nullptr) {
        //保存转化格式后的这一帧图片
        _image = new QImage((uchar *)data,spec.width,spec.height,QImage::Format_RGB888);
        //计算最终的尺寸
        //组件的尺寸
        int w = width();
        int h = height();
        //计算rect
        int dx = 0;
        int dy = 0;
        int dw = spec.width;
        int dh = spec.height;
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

//        qDebug() << "缩放后的视频尺寸矩形框" << dx << dy << dw << dh;
        _rect = QRect(dx,dy,dw,dh);
    }

    update();
}
void VideoWidget::freeImage(){
    if(_image){
        av_free(_image->bits());
        //C++中，new出来的是数据，释放时写成delete[]形式
//       delete[] _image->bits();
       delete  _image;
       _image = nullptr;
    }
}
