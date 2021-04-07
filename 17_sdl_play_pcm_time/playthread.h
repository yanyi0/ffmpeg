#ifndef PLAYTHREAD_H
#define PLAYTHREAD_H

#include <QObject>
#include <QThread>
#include <QDebug>

class PlayThread : public QThread
{
    Q_OBJECT
public:
    explicit PlayThread(QObject *parent = nullptr);
    ~PlayThread();
    void run() override;
    static void startTimerPlay();
    void startRemainPlay();

signals:
    void dataChanged(QString);
    void startPlay();
    void stopPlay();
};

#endif // PLAYTHREAD_H
