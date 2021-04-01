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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:{
      FFMPEG_HOME = ..
      INCLUDEPATH += -I $${FFMPEG_HOME}/include
      LIBS += -L $${FFMPEG_HOME}/lib \
         -lavutil \
         -lavcodec \
         -lavfilter \
         -lavformat \
         -lswscale \
         -lavdevice \
         -lpostproc \
         -lswresample \
         -lsvresample \
         -lswscale
}

linux:{
}

macx {
    FFMPEG_HOME = /usr/local/Cellar/ffmpeg/4.3.2
    message("- 系统路径 -" + $$(PATH))
    message("- macx -" + $${FFMPEG_HOME})
    # ffmpeg
    INCLUDEPATH += -I $${FFMPEG_HOME}/include
    LIBS += -L $${FFMPEG_HOME}/lib  \
         -lavutil \
         -lavcodec \
         -lavfilter \
         -lavformat \
         -lswscale \
         -lavdevice \
         -lpostproc \
         -lswresample \
         -lavresample \
         -lswscale
}

# 输出主程序路径
CONFIG(debug, debug|release){
    DESTDIR =$$PWD/bin_debug
} else {
    DESTDIR =$$PWD/bin_release
}
