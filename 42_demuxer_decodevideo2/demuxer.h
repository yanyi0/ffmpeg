#ifndef DEMUXER_H
#define DEMUXER_H
#include "QFile"
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

typedef struct {
    const char *filename;
    int sampleRate;
    AVSampleFormat sampleFmt;
    int chLayout;
} AudioDecodeSpec;

typedef struct {
    const char *filename;
    int width;
    int height;
    AVPixelFormat pixFmt;
    int fps;
} VideoDecodeSpec;

class Demuxer
{
public:
    Demuxer();
    void demux(const char *inFilename,AudioDecodeSpec &aOut,VideoDecodeSpec &vOut);
private:
    //解封装上下文
    AVFormatContext *_fmtCtx = nullptr;
    //解码上下文
    AVCodecContext *_aDecodeCtx = nullptr, *_vDecodeCtx = nullptr;
    //视频流，音频流
    AVStream *_aStream = nullptr,*_vStream = nullptr;
    //文件
    QFile _aOutFile,_vOutFile;
    //函数参数
    AudioDecodeSpec *_aOut = nullptr;
    VideoDecodeSpec *_vOut = nullptr;
    //存放解码后的数据
    AVFrame *_frame = nullptr;
    int initDecoder(AVCodecContext **decodeCtx,AVStream **stream,AVMediaType type);
    int initAudioInfo();
    int initVideoInfo();
    int decode(AVCodecContext *decodeCtx,AVPacket *pkt);
    void writeAudioFrame();
    void writeVideoFrame();
    int decode_packet(int *got_frame, int cached);
};

#endif // DEMUXER_H
