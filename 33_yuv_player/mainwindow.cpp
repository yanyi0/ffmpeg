#include "mainwindow.h"
#include "ui_mainwindow.h"
#ifdef Q_OS_WIN32
#define FILENAME "D:/音视频/TestMusic/RecordVideo/record640x480yuv.yuv"
#else
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/RecordVideo/out3.yuv"
#endif
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //创建播放器
    _player = new YuvPlayer(this);
    int w = 800;
    int h = 600;
    int x = (width()-w) >> 1;
    int y = (height()-h) >> 1;
    _player->setGeometry(x,y,w,h);

    //设置需要播放的文件
    Yuv yuv = {
       FILENAME,
        640,
        480,
        AV_PIX_FMT_YUYV422,
        30
    };
    _player->setYuv(yuv);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::on_playButton_clicked()
{
    if(_player->isPlaying()){//视频正在播放
        _player->pause();
        ui->playButton->setText("播放");
    }else{//视频没有正在播放
        _player->play();
        ui->playButton->setText("暂停");

    }
}

void MainWindow::on_stopButton_clicked()
{
//    _player->stop();
}
