#ifndef FFMPEGS_H
#define FFMPEGS_H

#include <QFile>
extern "C" {
//设备
#include <libavdevice/avdevice.h>
//格式
#include <libavformat/avformat.h>
//工具
#include <libavutil/avutil.h>
//重采样
#include <libswresample/swresample.h>
//编码
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

#define AV_ERROR(ret) \
    char errorbuf[1024] = {0};\
    av_strerror(ret,errorbuf,sizeof(errorbuf));
typedef struct {
    const char *filename;
    int width;
    int height;
    AVPixelFormat pixFmt;
    int fps;
} VideoDecodeSpec;

class FFmpegs
{
public:
    FFmpegs();
    static void h264Decode(const char *inFilename,VideoDecodeSpec &out);
};

#endif // FFMPEGS_H
