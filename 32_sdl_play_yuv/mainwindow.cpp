#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
//出错了就执行goto end
#define RET(judge,func) \
    if(judge){ \
    qDebug() << #func << "Error" << SDL_GetError(); \
    return; \
}
#ifdef Q_OS_WIN32
#define FILENAME "D:/音视频/TestMusic/RecordVideo/record640x480yuv.yuv"
#else
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/RecordVideo/out3.yuv"
#endif
#define PIXEL_FORMAT SDL_PIXELFORMAT_YUY2
#define IMG_W 640
#define IMG_H 480
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _widget = new QWidget(this);
    _widget->setGeometry(100,0,IMG_W,IMG_H);

    //初始化video子系统
    RET(SDL_Init(SDL_INIT_VIDEO),SDL_Init);

    //创建窗口
    _window = SDL_CreateWindowFrom((void *)_widget->winId());
    RET(!_window,SDL_CreateWindowFrom);
    //创建渲染上下文(默认的渲染目标是window)
    _renderer = SDL_CreateRenderer(_window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    //说明开启硬件加速失败
    if(!_renderer) _renderer = SDL_CreateRenderer(_window,-1,0);
    RET(!_renderer,SDL_CreateRenderer);
    //创建纹理
    _texture = SDL_CreateTexture(_renderer,PIXEL_FORMAT,SDL_TEXTUREACCESS_STREAMING,IMG_W,IMG_H);
    RET(!_texture,SDL_CreateTexture);

    //打开yuv文件
    _file.setFileName(FILENAME);
    if(!_file.open(QFile::ReadOnly)){
        qDebug() << "file open error" << FILENAME;
        return;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    //释放资源
    _file.close();
    SDL_DestroyTexture(_texture);
    SDL_DestroyRenderer(_renderer);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}
void MainWindow::on_playButton_clicked()
{
     //开启定时器，每秒钟播放30帧,1000/30 = 33.33333,每33毫秒播放一帧
    _timerId = startTimer(33);


}
//每隔一段时间就会调用
void MainWindow::timerEvent(QTimerEvent *event){
    //每一帧图片的大小
    int imgSize = IMG_W * IMG_H * 2;
    char data[imgSize];
    if(_file.read(data,imgSize) > 0){
        //将YUV数据填充到texture
        RET(SDL_UpdateTexture(_texture,nullptr,data,IMG_W),SDL_UpdateTexture);
        //设置红色的画笔颜色
        RET(SDL_SetRenderDrawColor(_renderer,0,0,0,SDL_ALPHA_OPAQUE),SDL_SetRenderDrawColor);
    //    //用绘制颜色(画笔颜色)DrawColor清除渲染目标
        RET(SDL_RenderClear(_renderer),SDL_RenderClear);
        //复制纹理数据到渲染目标(默认是window)上,从srcRect的大小,等比例复制到dstRect上,传NULL,表示全部的纹理大小渲染到全部的渲染目标上
        RET(SDL_RenderCopy(_renderer,_texture,nullptr,nullptr),SDL_RenderCopy);
        //将此前所有需要渲染的内容更新到屏幕上
        SDL_RenderPresent(_renderer);
    }else{//文件数据已经读取完毕
        //关闭定时器
        killTimer(_timerId);
    }

}
