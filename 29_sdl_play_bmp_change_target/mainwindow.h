#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <SDL2/SDL.h>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    SDL_Texture * createTexture(SDL_Renderer *renderer);
    void showClick(SDL_Event &event,SDL_Renderer *renderer,SDL_Texture *texture);
    ~MainWindow();

private slots:
    void on_playButton_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
