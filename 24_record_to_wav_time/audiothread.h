#ifndef AUDIOTHREAD_H
#define AUDIOTHREAD_H
#include <QObject>
#include <QDebug>
#include <QThread>
class AudioThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioThread(QObject *parent = nullptr);
    ~AudioThread();
    void setStop(bool stop);
private:
    bool _stop = false;
    void run();

signals:
    //当时间发生改变，算出来毫秒数传到主线程更新UI
    void timeChange(unsigned long long ms);
};

#endif // AUDIOTHREAD_H
