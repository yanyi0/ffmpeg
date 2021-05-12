#ifndef YUVPLAYER_H
#define YUVPLAYER_H

#include <QWidget>
#include <SDL2/SDL.h>
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
private:
    SDL_Window *_window = nullptr;
    SDL_Renderer *_renderer = nullptr;
    SDL_Texture *_texture = nullptr;
    Yuv _yuv;
    QFile _file;
    //保留播放器状态
    State _state = Stopped;
    int _timerId = 0;
    void timerEvent(QTimerEvent *event);
signals:

};

#endif // YUVPLAYER_H
