#include "playthread.h"
//出错了就执行goto end
#define END(judge,func) \
    if(judge){ \
    qDebug() << #func << "Error" << SDL_GetError(); \
    goto end; \
}
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
SDL_Texture* PlayThread::createTexture(SDL_Renderer *renderer){
    SDL_Texture *texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB24,SDL_TEXTUREACCESS_TARGET,50,50);
    if(!texture) return nullptr;
    //设置纹理为渲染目标
    if(SDL_SetRenderTarget(renderer,texture)) return nullptr;
    //设置矩形框背景颜色绿色
    if(SDL_SetRenderDrawColor(renderer,0,0,255,SDL_ALPHA_TRANSPARENT)) return nullptr;
    if(SDL_RenderClear(renderer)) return nullptr;
    //设置颜色
    if(SDL_SetRenderDrawColor(renderer,255,255,0,SDL_ALPHA_OPAQUE)) return nullptr;
    //画图形
    SDL_Rect rect = {0,0,50,50};
    if(SDL_RenderDrawRect(renderer,&rect)) return nullptr;
    //画线
    if(SDL_RenderDrawLine(renderer,0,0,50,50)) return nullptr;
    if(SDL_RenderDrawLine(renderer,50,0,0,50)) return nullptr;
    return texture;
}
void PlayThread::showClick(SDL_Event &event,SDL_Renderer *renderer,SDL_Texture *texture){
     SDL_MouseButtonEvent btn = event.button;
     //鼠标点击的位置的坐标
//     int x = btn.x;
//     int y = btn.y;
     int w = 0;
     int h = 0;
     if(SDL_QueryTexture(texture,nullptr,nullptr,&w,&h)) return;
     //鼠标点击的位置为矩形框的中心
     int x = btn.x - (w >> 1);
     int y = btn.y - (w >> 1);
     SDL_Rect dstRect = {x,y,w,h};
     //清除,不清除上一次的矩形框，会一直存在窗口上
     if(SDL_RenderClear(renderer)) return;
     //复制纹理到渲染目标
     SDL_RenderCopy(renderer,texture,nullptr,&dstRect);
     //画矩形框
//     SDL_SetRenderDrawColor(renderer,0,255,255,255);
//     SDL_RenderDrawRect(renderer,&dstRect);
     //更新渲染操作到屏幕上
     SDL_RenderPresent(renderer);
}
void PlayThread::run(){
    //窗口
    SDL_Window *window = nullptr;
    //渲染上下文
    SDL_Renderer *renderer = nullptr;
    //纹理(直接跟驱动程序相关的像素数据)
    SDL_Texture *texture = nullptr;
    //矩形框
    SDL_Rect dstRect = {100,100,50,50};
    //初始化video子系统
    END(SDL_Init(SDL_INIT_VIDEO),SDL_Init);
    //创建窗口
    window = SDL_CreateWindow(
                //窗口标题
                "SDL修改渲染目标",
                //窗口X
                SDL_WINDOWPOS_UNDEFINED,
                //窗口Y
                SDL_WINDOWPOS_UNDEFINED,
                //窗口宽度和图片宽度一致
                500,
                //窗口高度和图片高度一致
                500,
                //显示窗口
                SDL_WINDOW_SHOWN
                );
    END(!window,SDL_CreateWindow);
    //创建渲染上下文(默认的渲染目标是window)
    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    //说明开启硬件加速失败
    if(!renderer) {
        renderer = SDL_CreateRenderer(window,-1,0);
        END(!renderer,SDL_CreateRenderer);
    }
    //创建纹理
    texture = createTexture(renderer);
    END(!texture,createTexture);
    //设置渲染目标为window
    END(SDL_SetRenderTarget(renderer,nullptr),SDL_SetRenderTarget);
    //设置绘制颜色(画笔颜色)
    END(SDL_SetRenderDrawColor(renderer,0,0,0,SDL_ALPHA_OPAQUE),SDL_SetRenderDrawColor);
     //用绘制颜色(画笔颜色)DrawColor清除渲染目标
    END(SDL_RenderClear(renderer),SDL_RenderClear);
    //拷贝纹理到渲染目标上
    END(SDL_RenderCopy(renderer,texture,nullptr,&dstRect),SDL_RenderCopy);
    //将此前所有需要渲染的内容更新到屏幕上
    SDL_RenderPresent(renderer);
//     SDL_Delay(2000);
    //监听等待退出事件
    while(!isInterruptionRequested()){
        SDL_Event event;
        qDebug() << event.type << "事件";
        END(!SDL_WaitEvent(&event),SDL_WaitEvent);
                switch (event.type){
                case SDL_QUIT:{
                    goto end;
                    break;
                }
                   case SDL_MOUSEBUTTONUP:{
                    showClick(event,renderer,texture);
                    break;
                }
            }
         }
end:
    //释放资源
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
