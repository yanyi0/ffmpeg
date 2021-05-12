#ifndef PLAYTHREAD_H
#define PLAYTHREAD_H

#include <QObject>
#include <QThread>
#include <QDebug>

class PlayThread : public QThread
{
    Q_OBJECT
public:
    explicit PlayThread(void *winId,QObject *parent = nullptr);
    ~PlayThread();
    void run() override;
private:
    void *_winId;

signals:

};

#endif // PLAYTHREAD_H
