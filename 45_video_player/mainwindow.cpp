#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog"
#include "QDebug"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //创建播放器
    _player = new VideoPlayer();
    connect(_player,&VideoPlayer::stateChanged,this,&MainWindow::onPlayerStateChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::onPlayerStateChanged(VideoPlayer *player){
    VideoPlayer::State state = player->getState();
    if(state == VideoPlayer::Playing){
        ui->playBtn->setText("暂停");
    }else{
        ui->playBtn->setText("播放");
    }

    if(state == VideoPlayer::Stopped){
       ui->playBtn->setEnabled(false);
       ui->stopBtn->setEnabled(false);
       ui->currentSlider->setEnabled(false);
       ui->volumnSlider->setEnabled(false);
       ui->silenceBtn->setEnabled(false);

       ui->currentLabel->setText("00:00:00");
       ui->currentSlider->setValue(0);
       ui->volumnSlider->setValue(ui->volumnSlider->maximum());
    }else{
        ui->playBtn->setEnabled(true);
        ui->stopBtn->setEnabled(true);
        ui->currentSlider->setEnabled(true);
        ui->volumnSlider->setEnabled(true);
        ui->silenceBtn->setEnabled(true);
    }
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
                    "/",
                    "所有文件 (*.*);;"
                    "音频文件 (*.mp3 *.aac *.wav *.flac);;"
                    "视频文件 (*.mp4 *.avi *.mkv *.rmvb *.mov)");
        qDebug() << "打开文件path=" << filename;
        if(filename.isEmpty()) return;
        _player->setFilename(filename.toUtf8().data());
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

void MainWindow::on_currentSlider_valueChanged(int value)
{
    qDebug() << "on_currentSlider_valueChanged" << value;
    ui->currentLabel->setText(QString("%1").arg(value));
}

void MainWindow::on_volumnSlider_valueChanged(int value)
{
    qDebug() << "on_volumnSlider_valueChanged" << value;
    ui->volumnLabel->setText(QString("%1").arg(value));
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
