#include "playthread.h"
#include <QDebug>
#include <SDL2/SDL.h>
#include <QFile>

//采样率
#define SAMPLE_RATE 44100
//采样格式
#define SAMPLE_FORMAT 1
//采样声道
#define CHANNELS 2
//采样大小
#define SAMPLE_SIZE 32
//音频缓冲区的样本数量
#define SAMPLE 1024

#ifdef Q_OS_WIN32
#define FILENAME "D:/音视频/TestMusic/PlayPcm/out3.pcm"
#else
//文件路径
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayPcm/in111-s16le.pcm"
#endif

//音频缓冲区的大小
#define BUFFER_SIZE 4096

PlayThread::PlayThread(QObject *parent) : QThread(parent)
{
    //线程结束，自动删除
    connect(this,&PlayThread::finished,this,&PlayThread::deleteLater);
}
PlayThread::~PlayThread(){
    //析构后，自动解绑所有信号，防止再次删除
    disconnect();
    requestInterruption();
    quit();
    wait();
    qDebug() << this << "线程析构了";
}
int bufferLen;
char *bufferData;

//等待音频设备回调(会回调多次)
void pull_audio_data (void *userdata,
                      //需要往stream中填充PCM数据
                      Uint8 *stream,
                      //希望填充的大小(samples & format * channels/8)
                      int len){
     //清空stream
    SDL_memset(stream,0,len);
    //文件数据还没准备好
    if(bufferLen <= 0) return;
    //取len、bufferLen的最小值(为了保证数据安全，防止指针越界)
    len = len > bufferLen ? bufferLen:len;
    //填充数据
    SDL_MixAudio(stream,(Uint8 *)bufferData,len,SDL_MIX_MAXVOLUME);
    bufferData += len;
    bufferLen -= len;
}

/*
 SDL播放音频有两种方式
 Push(推):【程序】主动推送数据给【音频设备】
 Pull(拉): 【音频设备】主动向【程序】拉取数据
*/
void PlayThread::run(){
    qDebug() << "进入线程开始准备播放";
    //初始化Audio子系统
    if(SDL_Init(SDL_INIT_AUDIO)){
      qDebug() << "SDL_Init error" << SDL_GetError();
      return;
    }
    //音频参数
    SDL_AudioSpec spec;
    //采样率
    spec.freq = SAMPLE_RATE;
    //声道数
    spec.channels = CHANNELS;
    //采样格式(f32le)
    spec.format = AUDIO_S16LSB;
    //音频缓冲区的样本数量(这个值必须是2的幂)
    spec.samples = 1024;
    //回调
    spec.callback = pull_audio_data;
   //打开设备
    if(SDL_OpenAudio(&spec,nullptr)){
        qDebug() << "SDL_OpenAudio error" << SDL_GetError();
        //清除所有子系统
        SDL_Quit();
        return;
    }

    //打开文件
    QFile file(FILENAME);
    if(!file.open(QFile::ReadOnly)){
        //打开文件失败
        qDebug() << "file.open error" << FILENAME;
        //关闭设备
        SDL_CloseAudio();
        //清除所有子系统
        SDL_Quit();
        return;
    }

    //开始播放(0是取消暂停)
    SDL_PauseAudio(0);

    //存放从文件中读取的数据
    char data[BUFFER_SIZE];
    while(!isInterruptionRequested()){
        bufferLen = file.read(data,BUFFER_SIZE);
        if(bufferLen <= 0) break;
        //读取到了文件数据
        bufferData = data;
        //等待音频数据填充完毕
        //只要音频数据还没有填充完毕，就delay(sleep)
        while(bufferLen > 0){
            SDL_Delay(1);
        }
    }

    //清除所有的子系统
    SDL_Quit();
    //关闭文件
    file.close();
    //关闭音频设备
    SDL_CloseAudio();

}
