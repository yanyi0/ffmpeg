#include "videoplayer.h"
#pragma mark - 构造析构
VideoPlayer::VideoPlayer(QObject *parent) : QObject(parent)
{

}
VideoPlayer::~VideoPlayer(){

}
#pragma mark - 公有方法
VideoPlayer::State VideoPlayer::getState(){
     return _state;
}
void VideoPlayer:: play(){
    if(_state == VideoPlayer::Playing) return;
    //状态可能是暂停，停止，播放完毕
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
#pragma mark - 私有方法
void VideoPlayer::setState(State state){
    if(state == _state) return;
    _state = state;
    emit stateChanged(this);
}
