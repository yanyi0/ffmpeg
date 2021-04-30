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

};

#endif // AUDIOTHREAD_H
