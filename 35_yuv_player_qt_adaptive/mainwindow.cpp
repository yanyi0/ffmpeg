#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#ifdef Q_OS_WIN32
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PlayVideo/in.bmp"
#else
#define FILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/YuvToRGB/out.yuv"
#endif
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //创建播放器
    _player = new YuvPlayer(this);
    int w = 800;
    int h = 640;
    int x = (width()-w) >> 1;
    int y = (height()-h) >> 1;
    _player->setGeometry(x,y,w,h);

    //设置需要播放的文件
    Yuv yuv = {
        FILENAME,
        768,
        432,
        AV_PIX_FMT_YUV420P,
        23
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
        qDebug() << "点击暂停";
    }else{//视频没有正在播放
        _player->play();
        qDebug() << "点击开始播放";
        ui->playButton->setText("暂停");
    }
}

void MainWindow::on_stopButton_clicked()
{
    _player->stop();
}
