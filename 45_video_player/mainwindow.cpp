#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog"
#include "QDebug"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_stopBtn_clicked()
{
    int count = ui->playWidget->count();
    int idx = ui->playWidget->currentIndex();
    ui->playWidget->setCurrentIndex(++idx%count);
}

void MainWindow::on_openFileBtn_clicked()
{
    //文件夹路径
//        QString srcDirPath = QFileDialog::getExistingDirectory(
//                   this, "choose src Directory",
//                    "/");

//        if (srcDirPath.isEmpty())
//        {
//            return;
//        }
//        else
//        {
//            qDebug() << "srcDirPath=" << srcDirPath;
//            srcDirPath += "/";
//        }
        //文件路径
//        QString s = QFileDialog::getOpenFileName(
//                    this, "选择要播放的文件",
//                    "/",
//                    "视频文件 (*.flv *.rmvb *.avi *.mp4);; 所有文件 (*.*);; ");
//        qDebug() << "path=" << s;
//        if (!s.isEmpty())
//        {

//        }
    QString file_full, file_name, file_path;
    QFileInfo fi;

    file_full = QFileDialog::getOpenFileName(this);

    fi = QFileInfo(file_full);
    file_name = fi.fileName();
    file_path = fi.absolutePath();
    qDebug() << file_name << file_path;
}
