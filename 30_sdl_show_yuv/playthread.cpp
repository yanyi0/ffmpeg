#include "playthread.h"
#include <QDebug>
#include <SDL2/SDL.h>
#include <QFile>
//出错了就执行goto end
#define END(judge,func) \
    if(judge){ \
    qDebug() << #func << "Error" << SDL_GetError(); \
    goto end; \
}
#ifdef Q_OS_WIN32
#define FILENAME "D:/Dev/ffmpeg/ffmpeg/Picture/out_yuv420p.yuv"
#else
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayVideo/outyuv420p.yuv"
#endif
#define PIXEL_FORMAT SDL_PIXELFORMAT_IYUV
#define IMG_W 512
#define IMG_H 512
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
void PlayThread::run(){
    //窗口
    SDL_Window *window = nullptr;
    //渲染上下文
    SDL_Renderer *renderer = nullptr;
    //纹理(直接跟驱动程序相关的像素数据)
    SDL_Texture *texture = nullptr;
    //文件
    QFile file(FILENAME);
    //初始化video子系统
    END(SDL_Init(SDL_INIT_VIDEO),SDL_Init);

    //创建窗口
    window = SDL_CreateWindow(
                //窗口标题
                "SDL显示YUV图片",
                //窗口X
                SDL_WINDOWPOS_UNDEFINED,
                //窗口Y
                SDL_WINDOWPOS_UNDEFINED,
                //窗口宽度和图片宽度一致
                IMG_W,
                //窗口高度和图片高度一致
                IMG_H,
                //显示窗口
                SDL_WINDOW_SHOWN
                );
    END(!window,SDL_CreateWindow);
    //创建渲染上下文(默认的渲染目标是window)
    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    //说明开启硬件加速失败
    if(!renderer) renderer = SDL_CreateRenderer(window,-1,0);
    END(!renderer,SDL_CreateRenderer);
    //创建纹理
    texture = SDL_CreateTexture(renderer,PIXEL_FORMAT,SDL_TEXTUREACCESS_STREAMING,IMG_W,IMG_H);
    END(!texture,SDL_CreateTextureFromSurface);

    //打开yuv文件
    if(!file.open(QFile::ReadOnly)){
        qDebug() << "file open error" << FILENAME;
        goto end;
    }
    //将YUV数据填充到texture
    END(SDL_UpdateTexture(texture,nullptr,file.readAll().data(),IMG_W),SDL_UpdateTexture);
    //设置红色的画笔颜色
    END(SDL_SetRenderDrawColor(renderer,0,0,0,SDL_ALPHA_OPAQUE),SDL_SetRenderDrawColor);
//    //用绘制颜色(画笔颜色)DrawColor清除渲染目标
    END(SDL_RenderClear(renderer),SDL_RenderClear);
    //复制纹理数据到渲染目标(默认是window)上,从srcRect的大小,等比例复制到dstRect上,传NULL,表示全部的纹理大小渲染到全部的渲染目标上
    END(SDL_RenderCopy(renderer,texture,nullptr,nullptr),SDL_RenderCopy);
    //将此前所有需要渲染的内容更新到屏幕上
    SDL_RenderPresent(renderer);
    //监听等待退出事件
    while(!isInterruptionRequested()){
        SDL_Event event;
        qDebug() << event.type << "事件";
        END(!SDL_WaitEvent(&event),SDL_WaitEvent);
                switch (event.type){
                   case SDL_QUIT:
                   goto end;
                }
         }
end:
    //释放资源
    file.close();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
