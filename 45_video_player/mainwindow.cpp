#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog"
#include <QDebug>
#include <QMessageBox>

#pragma mark -构造，析构函数
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //注册信号的参数类型，保证能够发出信号
    qRegisterMetaType<VideoPlayer::VideoSwsSpec>("VideoSwsSpec&");
    //创建播放器
    _player = new VideoPlayer();
    connect(_player,&VideoPlayer::stateChanged,this,&MainWindow::onPlayerStateChanged);
    //时间进度条变化
    connect(_player,&VideoPlayer::timeChanged,this,&MainWindow::onPlayerTimeChanged);
    connect(_player,&VideoPlayer::initFinished,this,&MainWindow::onPlayerInitFinished);
    connect(_player,&VideoPlayer::playFailed,this,&MainWindow::onPlayerPlayFailed);
    connect(_player,&VideoPlayer::frameDecoded,ui->videoWidget,&VideoWidget::onPlayerFrameDecoded);
    //停止视频之后选择一个新的视频，播放器切换到VideoWidget，调用paint方法显示图片，里面的图片_image是上一个视频停止前的最后一张图片，且点击停止已经释放了_vSwsOutFrame
    //如果去渲染这张图片dramImage，会访问已经释放了的内存，导致闪退，也要监听VideoPlayer状态的改变，当为停止状态时，_image为上一个视频停止前的这一帧,要释放掉
    connect(_player,&VideoPlayer::stateChanged,ui->videoWidget,&VideoWidget::onPlayerStateChanged);

    //监听时间滑块点击
    connect(ui->timeSlider,&VideoSlider::clicked,this,&MainWindow::onSliderClicked);

    //设置音量滑块的范围
    ui->volumnSlider->setRange(VideoPlayer::Volumn::Min,VideoPlayer::Volumn::Max);
    //设置1/2音量
    ui->volumnSlider->setValue(ui->volumnSlider->maximum()>>1);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete _player;
}
#pragma mark -私有方法
void MainWindow::onSliderClicked(VideoSlider *slider){
   _player->setTime(slider->value());
}
void MainWindow::onPlayerInitFinished(VideoPlayer *player){
    qDebug() << player->getDuration();
    //设置slider进度条的伸缩范围
    ui->timeSlider->setRange(0,player->getDuration());
    //显示时间到label上面
    ui->durationLabel->setText(getTimeText(player->getDuration()));
}

void MainWindow::onPlayerTimeChanged(VideoPlayer *player){
    //音频播放时间设置滚动条和label
    ui->timeSlider->setValue(player->getTime());
}

void MainWindow::onPlayerStateChanged(VideoPlayer *player){
//    qDebug() << "-----------MainWindow::onPlayerStateChanged-----------";
    if(ui == nullptr) {
        qDebug() << "ui已经销毁了";
        return;
    }
    if(player == nullptr) {
        qDebug() << "player已经销毁了";
        return;
    }
    VideoPlayer::State state = player->getState();
    if(state == VideoPlayer::Playing){
        ui->playBtn->setText("暂停");
    }else{
        ui->playBtn->setText("播放");
    }

    if(state == VideoPlayer::Stopped){
       ui->playBtn->setEnabled(false);
       ui->stopBtn->setEnabled(false);
       ui->timeSlider->setEnabled(false);
       ui->volumnSlider->setEnabled(false);
       ui->muteBtn->setEnabled(false);

       ui->timeLabel->setText(getTimeText(0));
       ui->timeSlider->setValue(0);
       //显示打开文件的页面
       ui->playWidget->setCurrentWidget(ui->openFilePage);
    }else{
        ui->playBtn->setEnabled(true);
        ui->stopBtn->setEnabled(true);
        ui->timeSlider->setEnabled(true);
        ui->volumnSlider->setEnabled(true);
        ui->muteBtn->setEnabled(true);

//        qDebug() << "------什么情况导致黑屏--------";
        //显示播放的页面
        ui->playWidget->setCurrentWidget(ui->videoPage);
    }
}
void MainWindow::onPlayerPlayFailed(VideoPlayer *player){
   QMessageBox::critical(nullptr,"提示","哦豁，播放失败!");
}
void MainWindow::on_stopBtn_clicked()
{
    _player->stop();
//    int count = ui->playWidget->count();
//    int idx = ui->playWidget->currentIndex();
//    ui->playWidget->setCurrentIndex(++idx%count);
}

void MainWindow::on_openFileBtn_clicked()
{
        //文件路径
        //选取单个文件
        QString filename = QFileDialog::getOpenFileName(
                    this, "选择要播放的文件",
                    "/Users/cloud/Documents/iOS/AudioVideo/TestMusic/素材/",
                    "所有文件 (*.*);;"
                    "多媒体文件 (*.mp3 *.aac *.wav *.flac *.mp4 *.avi *.mkv *.rmvb *.mov)");
        qDebug() << "打开文件path=" << filename;
        if(filename.isEmpty()) return;
//        qDebug() << "------文件名-------" << filename.toUtf8().data();
        //如果出现乱码，转成标准库中的str
//        std::string str = filename.toStdString();
//        const char* ch = str.c_str();
        //开始播放打开的文件
        _player->setFilename(filename);
        _player->play();
        //选取多个文件
//        QStringList ss = QFileDialog::getOpenFileNames(
//                    this, "选择要播放的文件",
//                    "/",
//                    "所有文件 (*.*);;"
//                    "音频文件 (*.mp3 *.aac *.wav *.flac);;"
//                    "视频文件 (*.mp4 *.avi *.mkv *.rmvb *.mov)");
//        foreach(QString string,ss)
//        qDebug() << string;
//    QString file_full, file_name, file_path;
//    QFileInfo fi;

//    file_full = QFileDialog::getOpenFileName(this);

//    fi = QFileInfo(file_full);
//    file_name = fi.fileName();
//    file_path = fi.absolutePath();
//    qDebug() << file_name << file_path;
}

void MainWindow::on_timeSlider_valueChanged(int value)
{
    ui->timeLabel->setText(getTimeText(value));
//    ui->currentLabel->setText("00:00:00");
//    ui->currentLabel->setText(QString("%1").arg(value));
}

void MainWindow::on_volumnSlider_valueChanged(int value)
{
    qDebug() << "on_volumnSlider_valueChanged" << value;
    ui->volumnLabel->setText(QString("%1").arg(value));
    _player->setVolumn(value);
}

void MainWindow::on_playBtn_clicked()
{
    VideoPlayer::State state = _player->getState();
    if(state == VideoPlayer::Playing){
       _player->pause();
    }else{
       _player->play();
    }
}
QString MainWindow::getTimeText(int value){
    //    int seconds = 5000;
    //    int h = seconds/3600;
    //    int m = seconds%3600/60;
    //    int m = seconds/60%60;
    //    int s = seconds%60;
//    int64_t seconds = value / 1000000;
    //前面补0表明至少是两位，后面取最右边两位
//    QString h = QString("0%1").arg(seconds/3600).right(2);
//    QString m = QString("0%1").arg(seconds%3600/60).right(2);
//    QString s = QString("0%1").arg(seconds%60).right(2);
//    return QString("%1:%2:%3").arg(h).arg(m).arg(s);

    //QString进行格式化
    QLatin1Char fillChar = QLatin1Char('0');
    return QString("%1:%2:%3")
            .arg(value/3600,2,10,fillChar)
            .arg(value%3600/60,2,10,fillChar)
            .arg(value%60,2,10,fillChar);
}

void MainWindow::on_muteBtn_clicked()
{
    if(_player->isMute()){
        _player->setMute(false);
        ui->muteBtn->setText("静音");
    }else{
        _player->setMute(true);
        ui->muteBtn->setText("开音");
    }
    ui->muteBtn->repaint();
}
