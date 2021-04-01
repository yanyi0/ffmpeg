#include "playthread.h"
#include <QDebug>
#include <SDL2/SDL.h>
#include <QFile>

//采样率
#define SAMPLE_RATE 44100
//采样格式
#define SAMPLE_FORMAT AUDIO_F32LSB
//采样大小
//#define SAMPLE_SIZE 16
#define SAMPLE_SIZE SDL_AUDIO_BITSIZE(SAMPLE_FORMAT)
//采样声道
#define CHANNELS 2

//音频缓冲区的样本数量
#define SAMPLES 1024
//4096*8是音频缓冲区的极限样本数量，若再大程序直接退出
//#define SAMPLES 4096*8
//文件路径
//#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayPcm/in111-s16le.pcm"
//#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayPcm/in-f32le.pcm"
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayPcm/in1-f32le.pcm"
//#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayPcm/myheart-f32le-new.pcm"
//#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayPcm/myheart-s32le-new.pcm"
//#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayPcm/myheart-s16le-new.pcm"
//#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayPcm/StayHereForever-f32le.pcm"
//#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayPcm/Perfect-f32le.pcm"
//每个样本占用多少个字节
#define BYTES_PER_SAMPLE ((SAMPLE_SIZE * CHANNELS)/8)
//相当于右移3位
//#define BYTES_PER_SAMPLE ((SAMPLE_SIZE * CHANNELS) >> 3)
//音频缓冲区的大小
//#define BUFFER_SIZE 4096
#define BUFFER_SIZE (SAMPLES * BYTES_PER_SAMPLE)

typedef struct {
    int len = 0;
    //每次音频缓冲区拉取的数量
    int pullLen = 0;
    Uint8 *data = nullptr;
} AudioBuffer;

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


//等待音频设备回调(会回调多次)
void pull_audio_data (void *userdata,
                      //需要往stream中填充PCM数据
                      Uint8 *stream,
                      //希望填充的大小(samples * format * channels/8)
                      int len){
    qDebug() << "pull_audio_data" << len;
     //清空stream
    SDL_memset(stream,0,len);
    //取出AudioBuffer
    AudioBuffer *audioBuffer = (AudioBuffer *)userdata;
    //文件数据还没准备好
    if(audioBuffer->len <= 0) return;
    //取len、bufferLen的最小值(为了保证数据安全，防止指针越界)
    audioBuffer->pullLen = len > audioBuffer->len ? audioBuffer->len:len;
    //填充数据
    SDL_MixAudio(stream,(Uint8 *)audioBuffer->data,audioBuffer->pullLen,SDL_MIX_MAXVOLUME);
    audioBuffer->data += audioBuffer->pullLen;
    audioBuffer->len -=  audioBuffer->pullLen;
    //若最后一次读取的数据正好是音频缓冲区的数据的大小，且是很大的数据，播放器需要长一点的时间去播放，
    //而此时 audioBuffer->len -= len已经正好减为0了，audioBuffer->len 为0,而此时继续就会再去文件中读取数据，
    //而文件中读取到的audioBuffer.len为0，则此时会执行if(audioBuffer.len <= 0) break;跳出循环，关闭播放器，直接退出，到时音频播放到一半就退出了
    //解决方案 计算剩余的时间，进行延迟退出while循环
    //音频缓冲区的大小越大，出现音乐没有播放完毕就直接退出的可能性越大，
    //比如音频缓冲区的大且最后一次是读的比较多的数据，足够播放一分钟才能播放完毕，此时读取的数据又为0，执行break，跳出循环，关闭播放设备和文件
    //音频缓冲区越大，点击开始播放的响应时间越长，需要等一会才会开始播放，先要将缓冲区塞满才会调用回调函数，从而变慢
    //若音频缓冲区的样本数量继续增大，则达到一定值，会有临界值，超过临界值就会默认1024*32*2/8，即不可能无限大
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
    spec.format = AUDIO_F32LSB;
    //音频缓冲区的样本数量(这个值必须是2的幂)
    spec.samples = SAMPLES;
    //回调
    spec.callback = pull_audio_data;

    AudioBuffer audioBuffer;
    spec.userdata = &audioBuffer;

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
    Uint8 data[BUFFER_SIZE];
    while(!isInterruptionRequested()){
        //等待音频数据填充完毕
        //只要音频数据还没有填充完毕，就跳过
        if(audioBuffer.len > 0) continue;
        audioBuffer.len = file.read((char *)data,BUFFER_SIZE);
        //文件数据已经读取完毕
        if(audioBuffer.len <= 0) {
            //剩余的样本数量
            int samples = audioBuffer.pullLen / BYTES_PER_SAMPLE;
            int ms = samples * 1000 /SAMPLE_RATE;
            SDL_Delay(ms);
            qDebug() << ms;
            break;
        }
        //读取到了文件数据
        audioBuffer.data = data;
    }
//    while(!isInterruptionRequested()) {
//        bufferLen = file.read(data,BUFFER_SIZE);
//        if(bufferLen <= 0) break;
//        //读取到了文件数据
//        bufferData = data;
//        //等待音频数据填充完毕
//        //只要音频数据还没有填充完毕，就delay(sleep)
//        while(bufferLen > 0){
//            SDL_Delay(1);
//        }
//    }

    //清除所有的子系统
    SDL_Quit();
    //关闭文件
    file.close();
    //关闭音频设备
    SDL_CloseAudio();

}
