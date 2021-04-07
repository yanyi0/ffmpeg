#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
    QStringList timeCounter();
    QString timeCounterString();
private slots:
    void on_startButton_clicked();

    void on_stopButton_clicked();

    void slot_timer_timeout();

private:
    Ui::MainWindow *ui;
    QElapsedTimer *time;
    QTimer *timer;
};
#endif // MAINWINDOW_H
