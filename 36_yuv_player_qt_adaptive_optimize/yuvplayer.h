#ifndef YUVPLAYER_H
#define YUVPLAYER_H

#include <QWidget>
#include <QFile>
extern "C" {
 #include <libavutil/avutil.h>
}
typedef struct {
   const char *filename;
   int width;
   int height;
   AVPixelFormat pixelFmt;
   int fps;
} Yuv;
class YuvPlayer : public QWidget
{
    Q_OBJECT
public:
    //播放器状态
    typedef enum {
        Stopped = 0,
        Playing,
        Paused,
        Finished
    } State;
    explicit YuvPlayer(QWidget *parent = nullptr);
    ~YuvPlayer();
    void play();
    void pause();
    void stop();
    bool isPlaying();

    void setYuv(Yuv &yuv);
    State getState();
signals:
    void stateChanged();
private:
    Yuv _yuv;
    QFile *_file;
    //保留播放器状态
    State _state = Stopped;
    int _timerId = 0;
    QImage *_currentImage = nullptr;
    //视频矩形框
    QRect _dstRect;
    //一帧图片的大小
    int  _imgSize;
    //刷帧的时间间隔
    int _interval;
    void timerEvent(QTimerEvent *event);
    void paintEvent(QPaintEvent *event);
    //释放当前这一帧图片
    void freeCurrentImage();
    //释放定时器
    void stopTimer();
    //改变状态
    void setState(State state);
    //关闭文件
    void closeFile();
};

#endif // YUVPLAYER_H
