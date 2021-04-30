#include "playthread.h"
#include <QDebug>
#include <SDL2/SDL.h>
#include <QFile>


#ifdef _WIN32
#else
//文件路径
#define FILENAME "/Users/cloud/documents/iOS/音视频/TestMusic/PlayWav/Myheartwillgoon3.wav"
#endif

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
    //加载wav文件
    SDL_AudioSpec spec;
    //指向PCM数据
    Uint8 *data = nullptr;
    //PCM长度
    Uint32 len = 0;
    if(!SDL_LoadWAV(FILENAME,&spec,&data,&len)){
       qDebug() << "SDL_LoadWAV error" << SDL_GetError();
       //清除所有子系统
       SDL_Quit();
       return;
    }
    //一次性读取wav文件中的所有数据到内存
    qDebug() << spec.freq << spec.channels << len << data;
    //音频缓冲区的样本数量
    spec.samples = 1024;
    //回调 放到后面设置，若在前面设置&spec传入SDL_LoadWAV,会重新覆盖掉callback的值
    spec.callback = pull_audio_data;

    AudioBuffer audioBuffer;
    //所有的PCM数据
    audioBuffer.data = data;
    //所有的PCM数据长度
    audioBuffer.len = len;
    spec.userdata = &audioBuffer;

   //打开设备
    if(SDL_OpenAudio(&spec,nullptr)){
        qDebug() << "SDL_OpenAudio error" << SDL_GetError();
        //清除所有子系统
        SDL_Quit();
        return;
    }
    //开始播放(0是取消暂停)
    SDL_PauseAudio(0);
    int sampleSize = SDL_AUDIO_BITSIZE(AUDIO_F32LSB);
    int bytesPerSample = (sampleSize * spec.channels)/8;

    while(!isInterruptionRequested()){
        //等待音频数据填充完毕
        //只要音频数据还没有填充完毕，就跳过
        if(audioBuffer.len > 0) continue;
        //文件数据已经读取完毕
        if(audioBuffer.len <= 0) {
            //剩余的样本数量
            int samples = audioBuffer.pullLen / bytesPerSample;
            int ms = samples * 1000 /spec.freq;
            SDL_Delay(ms);
            qDebug() << ms;
            break;
        }
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

    //释放WAV文件数据
    SDL_FreeWAV(data);
    //清除所有的子系统
    SDL_Quit();
    //关闭音频设备
    SDL_CloseAudio();

}
