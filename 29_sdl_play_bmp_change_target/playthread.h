#ifndef PLAYTHREAD_H
#define PLAYTHREAD_H
#include <SDL2/SDL.h>
#include <QObject>
#include <QThread>
#include <QDebug>

class PlayThread : public QThread
{
    Q_OBJECT
public:
    explicit PlayThread(QObject *parent = nullptr);
    SDL_Texture * createTexture(SDL_Renderer *renderer);
    void showClick(SDL_Event &event,SDL_Renderer *renderer,SDL_Texture *texture);
    ~PlayThread();
    void run() override;

signals:

};

#endif // PLAYTHREAD_H
