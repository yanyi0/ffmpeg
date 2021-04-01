#include "mainwindow.h"
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QPushButton *btn = new QPushButton;
    btn->setText("登录");
    btn->setFixedSize(100,30);
    btn->setParent(this);
    connect(btn,&QPushButton::clicked,this,&MainWindow::close);
}

MainWindow::~MainWindow()
{
}

