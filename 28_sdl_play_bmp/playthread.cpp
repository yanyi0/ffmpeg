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

#define FILEPATH "D:/Dev/ffmpeg/ffmpeg/videopicture/in.bmp"
#else
#define FILEPATH "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayVideo/in.bmp"
#endif

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
    //像素数据
    SDL_Surface *surface = nullptr;
    //纹理(直接跟驱动程序相关的像素数据)
    SDL_Texture *texture = nullptr;
    //时间
    SDL_Event event;
    //矩形框:从给出的图片的x=250,y=260位置开始绘制一个正方形大小100新100放大到512x512屏幕上，图像眼睛的区域
//    SDL_Rect srcRect = {250,260,100,100};
//    SDL_Rect dstRect = {0,0,512,512};
    //缩放，将整个512x512的区域缩放到x=200,y=200,100x100的正方形区域
    SDL_Rect srcRect = {0,0,512,512};
    SDL_Rect dstRect = {200,200,100,100};

    SDL_Rect rect;

    //初始化video子系统
    END(SDL_Init(SDL_INIT_VIDEO),SDL_Init);
    //加载BMP
    surface = SDL_LoadBMP(FILEPATH);
    END(!surface,SDL_LoadBMP);
    //创建窗口
    window = SDL_CreateWindow(
                //窗口标题
                "SDL显示BMP图片",
                //窗口X
                SDL_WINDOWPOS_UNDEFINED,
                //窗口Y
                SDL_WINDOWPOS_UNDEFINED,
                //窗口宽度和图片宽度一致
                surface->w,
                //窗口高度和图片高度一致
                surface->h,
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
    texture = SDL_CreateTextureFromSurface(renderer,surface);
    END(!texture,SDL_CreateTextureFromSurface);
    //设置红色的画笔颜色
    END(SDL_SetRenderDrawColor(renderer,255,0,0,SDL_ALPHA_OPAQUE),SDL_SetRenderDrawColor);
    //画一个红色的矩形框，窗口的左上角正方形50x50
    rect = {0,0,50,50};
    //实心矩形框
    SDL_RenderFillRect(renderer,&rect);
    //空心矩形框
//    SDL_RenderDrawRect(renderer,&rect);
//    //设置绘制颜色(这里随便设置了一个颜色:黄色)
    END(SDL_SetRenderDrawColor(renderer,255,255,0,SDL_ALPHA_OPAQUE),SDL_SetRenderDrawColor);
//    //用绘制颜色(画笔颜色)DrawColor清除渲染目标
    END(SDL_RenderClear(renderer),SDL_RenderClear);
    //复制纹理数据到渲染目标(默认是window)上,null表示默认充满整个纹理
    END(SDL_RenderCopy(renderer,texture,nullptr,nullptr),SDL_RenderCopy);
//    END(SDL_RenderCopy(renderer,texture,&srcRect,&dstRect),SDL_RenderCopy);
    //将此前所有需要渲染的内容更新到屏幕上
    SDL_RenderPresent(renderer);
    //监听退出事件
    while(!isInterruptionRequested()){
        qDebug() << "事件" << event.type;
        END(!SDL_WaitEvent(&event),SDL_WaitEvent)
                switch (event.type) {
                case SDL_QUIT:{
                goto end;
                break;
            }
        }
    }
end:
    ;
    //释放资源
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
