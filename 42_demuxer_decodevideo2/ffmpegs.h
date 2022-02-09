#ifndef FFMPEGS_H
#define FFMPEGS_H
extern "C" {
//格式
#include <libavformat/avformat.h>
//工具
#include <libavutil/avutil.h>
#include <libavutil/samplefmt.h>
//编码
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
}
class FFmpegs
{
public:
    FFmpegs();
    static void demuxer();
};

#endif // FFMPEGS_H
