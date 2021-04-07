#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QThread>
#include "ffmpegs.h"
#include <QFile>
extern "C" {
//设备
#include <libavdevice/avdevice.h>
}
//采样率
#define SAMPLERATE 44100
//声道
#define NUMBERCHANNELS 2
#ifdef Q_OS_WIN32
#define BITSPERSAMPLE 16
#else
#define BITSPERSAMPLE 32
#define PCMFILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PcmToWav/Myheartwillgoon3.pcm"
#define WAVFILENAME "/Users/cloud/Documents/iOS/音视频/TestMusic/PcmToWav/Myheartwillgoon3-f32le-audioformat3.wav"
#endif
int main(int argc, char *argv[])
{
    WAVHeader wavHead;

    qDebug() << sizeof (WAVHeader);
//    //文件总大小减8
//    wavHead.chunkDataSize = 3676288 + 44 - 8;
    //声道 2
    wavHead.nubmerChannels = NUMBERCHANNELS;
    //采样率
    wavHead.sampleRate = SAMPLERATE;
    //字节率
    wavHead.byteRate = SAMPLERATE * BITSPERSAMPLE * 2/8;
    //一个样本所占的字节数
    wavHead.blockAlign = BITSPERSAMPLE * 2 /8;
    //位深度
    wavHead.bitsPerSample = BITSPERSAMPLE;
//    //dataChunkDataSize
//    wavHead.dataChunkDataSize = 3676288;
    //pcm为1， 浮点型为3
    wavHead.audioFormat = 3;

    QFile pcmFile(PCMFILENAME);
    int allSize = pcmFile.size();

    qDebug() << "文件总大小" << allSize;
     //dataChunkDataSize
    wavHead.dataChunkDataSize = allSize;
    //文件总大小减8
    wavHead.chunkDataSize = allSize + 44 - 8;

    FFmpegs::pcmToWav(wavHead,PCMFILENAME,WAVFILENAME);


    qDebug() << "main" << QThread::currentThread();
    //注册设备
    avdevice_register_all();
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
