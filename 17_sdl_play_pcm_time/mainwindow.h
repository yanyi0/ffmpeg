#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <playthread.h>
#include <QTime>
#include <QTimer>
#include <QElapsedTimer>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QString timeCounterString();

private slots:
    void on_playButton_clicked();
    void changeButtonText(QString str);
    void timerStartText();
    void slot_timer_timeout();
    void timerStopText();

private:
    Ui::MainWindow *ui;
    PlayThread *_playThread = nullptr;
    QElapsedTimer *time;
    QTimer *timer;
};
#endif // MAINWINDOW_H
