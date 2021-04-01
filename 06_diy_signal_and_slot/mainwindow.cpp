#include "mainwindow.h"
#include "sender.h"
#include "receiver.h"
#include <QDebug>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    Sender *sender = new Sender;
    Receiver *receiver = new Receiver;
    connect(sender,&Sender::exit,receiver,&Receiver::handleExit);

    qDebug() << emit sender->exit(10,20);
    delete sender;
    delete receiver;
}

MainWindow::~MainWindow()
{
}

