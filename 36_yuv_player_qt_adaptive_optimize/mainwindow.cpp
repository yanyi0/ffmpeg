#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#ifdef Q_OS_WIN32
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayVideo/in.bmp"
#else
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/out.yuv"
#endif
static int yuvIdx = 0;
static Yuv yuvs[] = {
    {
        FILENAME,
        768,
        432,
        AV_PIX_FMT_YUV420P,
        23
    },
    {
       "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/outyuv420p.yuv",
        512,
        512,
        AV_PIX_FMT_YUV420P,
        30
    },
    {
       "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/out3_uyvy422_1280x720.yuv",
        1280,
        720,
        AV_PIX_FMT_UYVY422,
        30
    },
    {
       "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/InTheEnd.yuv",
        480,
        320,
        AV_PIX_FMT_YUV420P,
        30
    },
    {
       "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/out1_yuyv422_1280x720.yuv",
        1280,
        720,
        AV_PIX_FMT_YUYV422,
        30
    }
};
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //创建播放器
    _player = new YuvPlayer(this);
    //监听播放器
    connect(_player,&YuvPlayer::stateChanged,this,&MainWindow::onPlayerStateChanged);
    //设置播放器位置和尺寸
    int w = 800;
    int h = 640;
    int x = (width()-w) >> 1;
    int y = (height()-h) >> 1;
    _player->setGeometry(x,y,w,h);

    //设置需要播放的文件
    _player->setYuv(yuvs[yuvIdx]);
    _player->play();
}
void MainWindow::onPlayerStateChanged(){
    if(_player->getState() == YuvPlayer::Playing){
        ui->playButton->setText("暂停");
    }else{
        ui->playButton->setText("播放");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_playButton_clicked()
{
    if(_player->isPlaying()){//视频正在播放
        _player->pause();
        qDebug() << "点击暂停";
    }else{//视频没有正在播放
        _player->play();
        qDebug() << "点击开始播放";
    }
}

void MainWindow::on_stopButton_clicked()
{
    _player->stop();
}
/*
* QFile::setFileName: File (/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/out.yuv) already open
* 点击下一步重新设置文件名fileName后，因为_file是成员变量存在于YuvPlayer内存中，只有YuvPlayer销毁的时候，才会被销毁
* 所以播放器不死，就不能装载新的文件
* 设置成指针变量*_file的时候，可以随时销毁_file指向的内存空间
*/
void MainWindow::on_nextButton_clicked()
{
    //总共数组中有多少个视频
    int yuvCount = sizeof(yuvs)/sizeof(Yuv);
    //取模运算防止数组越界
    yuvIdx = ++yuvIdx % yuvCount;
    _player->stop();
    //点击下一个设置当前的yuv
    _player->setYuv(yuvs[yuvIdx]);
    _player->play();
}
