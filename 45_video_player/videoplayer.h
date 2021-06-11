#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QObject>
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
private:
    //保留播放器当前状态
    State _state = Stopped;
    //改变状态
    void setState(State state);
    /* 文件名 */
    const char *_filename;
signals:
   void stateChanged(VideoPlayer *player);
};

#endif // VIDEOPLAYER_H
