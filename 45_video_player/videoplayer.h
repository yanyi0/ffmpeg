#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QObject>
extern "C" {
//格式
#include <libavformat/avformat.h>
//工具
#include <libavutil/avutil.h>
//编码
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}
/*
 * 预处理视频，不负责显示、渲染视频
 */
class VideoPlayer : public QObject
{
    Q_OBJECT
public:
    //播放器状态
    typedef enum {
        Stopped = 0,
        Playing,
        Paused
    } State;
    explicit VideoPlayer(QObject *parent = nullptr);
    ~VideoPlayer();
    /* 播放 */
    void play();
    /* 暂停 */
    void pause();
    /* 停止 */
    void stop();
    /* 是否正在播放中 */
    bool isPlaying();
    /* 获取当前状态 */
    State getState();
    /* 设置文件名 */
    void setFilename(const char *filename);
    /* 获取总时长(单位是微秒，1秒 = 10^3毫秒 = 10^6微妙) */
    int64_t getDuration();
private:
    //解封装上下文
    AVFormatContext *_fmtCtx = nullptr;
    //解码上下文
    AVCodecContext *_aDecodeCtx = nullptr, *_vDecodeCtx = nullptr;
    //视频流，音频流
    AVStream *_aStream = nullptr,*_vStream = nullptr;
    //存放解码后的数据
    AVFrame *_aframe = nullptr,*_vframe = nullptr;

    int initDecoder(AVCodecContext **decodeCtx,AVStream **stream,AVMediaType type);
    int initAudioInfo();
    int initVideoInfo();

    //保留播放器当前状态
    State _state = Stopped;
    //改变状态
    void setState(State state);
    /* 文件名 */
    const char *_filename;
    /* 读取文件 */
    void readFile();
signals:
   void stateChanged(VideoPlayer *player);
   void initFinished(VideoPlayer *player);
   void playFailed(VideoPlayer *player);
};

#endif // VIDEOPLAYER_H
