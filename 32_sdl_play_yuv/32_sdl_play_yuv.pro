QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32 {
   SDL_HOME = D:/FFMpeg/SDL2-2.0.14/x86_64-w64-mingw32
   FFMPEG_HOME = D:/FFMpeg/ffmpeg-4.3.2
}

macx {
   SDL_HOME = /usr/local/Cellar/sdl2/2.0.14_1
   FFMPEG_HOME = /usr/local/ffmpeg
}

INCLUDEPATH += $${SDL_HOME}/include
LIBS += -L$${SDL_HOME}/lib \
        -lSDL2

INCLUDEPATH += $${FFMPEG_HOME}/include
LIBS += -L$${FFMPEG_HOME}/lib \
        -lavcodec \
        -lavdevice \
        -lavformat \
        -lavutil