#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QObject>
#include "list"
#include "condmutex.h"
#include "QThread"

extern "C" {
//格式
#include <libavformat/avformat.h>
//工具
#include <libavutil/avutil.h>
//编码
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
//重采样
#include <libswresample/swresample.h>
//像素格式转换
#include <libswscale/swscale.h>
}
#include <QDebug>
#define ERROR_BUF \
    char errbuf[1024];\
    av_strerror(ret,errbuf,sizeof(errbuf));

#define CODE(func,code) \
    if(ret < 0) { \
    ERROR_BUF;\
    qDebug() << #func << "error" << errbuf;\
    code;\
    }

#define END(func) CODE(func,fataError();return;)

#define RET(func) CODE(func,return ret;)

#define CONTINUE(func) CODE(func,continue;)
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
    //音量
    typedef enum {
        Min = 0,
        Max = 100
    } Volumn;
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
    void setFilename(QString &filename);
    /* 获取总时长(单位是微秒，1秒 = 10^3毫秒 = 10^6微妙) */
    int64_t getDuration();
    /* 设置音量 */
    void setVolumn(int volumn);
    /* 获取当前音量 */
    int getVolume();
    /* 设置静音 */
    void setMute(bool mute);
    bool isMute();
private:
    /************** 音频相关 **************/
    typedef struct {
        int sampleRate;
        AVSampleFormat sampleFmt;
        int chLayout;
        int chs;
        //每一个样本帧(两个声道(左右声道))的大小
        int bytesPerSampleFrame;
    } AudioSwrSpec;

    //解码上下文
    AVCodecContext *_aDecodeCtx = nullptr;
    //音频流
    AVStream *_aStream = nullptr;
    //存放音频包的列表  ---- 跟随VideoPlayer生而生死而死，用对象不用指针
    std::list<AVPacket> _aPktList;
    //音频包列表的锁 ---- 跟随VideoPlayer生而生死而死，用对象不用指针
    CondMutex _aMutex;
    //音频重采样上下文
    SwrContext *_aSwrCtx = nullptr;
    //音频重采样输入/输出参数
    AudioSwrSpec _aSwrInSpec,_aSwrOutSpec;
    //存放解码后的音频重采样输入/输出数据
    AVFrame *_aSwrInFrame = nullptr,*_aSwrOutFrame = nullptr;
    //从哪个位置开始取出PCM数据填充到SDL的音频缓冲区，应对复杂情况，PCM大于缓冲区，需要二次填充记录已经填充的PCM的索引位置
    int _aSwrOutIdx = 0;
    //音频重采样后输出的PCM数据大小
    int _aSwrOutSize = 0;
    //音量
    int _volumn = Max;
    //经营
    bool _mute = false;
    //初始化音频信息
    int initAudioInfo();
    //初始化SDL
    int initSDL();
    //初始化音频重采样
    int initSwr();
    //添加数据包到音频列表中
    void addAudioPkt(AVPacket &pkt);
    //清空音频数据包列表
    void clearAudioPktList();
    //SDL填充缓冲区的回调函数
    static void sdlAudioCallbackFunc(void *userdata,Uint8 *stream,int len);
    //SDL填充缓冲区的回调函数
    void sdlAudioCallback(Uint8 *stream,int len);
    //音频解码
    int decodeAudio();

    /************** 视频相关 *************/
    //解码上下文
    AVCodecContext *_vDecodeCtx = nullptr;
    //视频流
    AVStream *_vStream = nullptr;
    //存放解码后的视频像素格式输入/输出数据
    AVFrame *_vSwsInFrame = nullptr,*_vSwsOutFrame = nullptr;
    //像素格式转换上下文
    SwsContext *_vSwsCtx = nullptr;
    //存放视频包的列表 ---- 跟随VideoPlayer生而生死而死，用对象不用指针
    std::list<AVPacket> _vPktList;
    //视频包列表的锁 ---- 跟随VideoPlayer生而生死而死，用对象不用指针
    CondMutex _vMutex;
    //初始化视频信息
    int initVideoInfo();
    //初始化视频像素格式转换
    int initSws();
    //添加数据包到视频列表中
    void addVideoPkt(AVPacket &pkt);
    //清空视频数据包列表
    void clearVideoPktList();
    //解码视频
    void decodeVideo();

    /***** 其他 *****/
    //解封装上下文
    AVFormatContext *_fmtCtx = nullptr;

    int initDecoder(AVCodecContext **decodeCtx,AVStream **stream,AVMediaType type);

    //保留播放器当前状态
    State _state = Stopped;
    //改变状态
    void setState(State state);
    /* 文件名 */
    char _filename[512];
    /* 读取文件 */
    void readFile();
    //释放资源
    void free();
    void freeAudio();
    void freeVideo();
    void fataError();
signals:
   void stateChanged(VideoPlayer *player);
   void initFinished(VideoPlayer *player);
   void playFailed(VideoPlayer *player);
};

#endif // VIDEOPLAYER_H
