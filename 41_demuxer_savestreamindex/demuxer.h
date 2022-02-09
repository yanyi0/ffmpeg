#ifndef DEMUXER_H
#define DEMUXER_H
#include "QFile"
extern "C" {
//格式
#include <libavformat/avformat.h>
//工具
#include <libavutil/avutil.h>
//编码
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
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
//    AVStream *_aStream = nullptr,*_vStream = nullptr;
    //流索引
    int _aStreamIdx = 0,_vStreamIdx = 0;
    //文件
    QFile _aOutFile,_vOutFile;
    //函数参数
    AudioDecodeSpec *_aOut = nullptr;
    VideoDecodeSpec *_vOut = nullptr;
    //存放解码后的数据
    AVFrame *_frame = nullptr;
//    int initDecoder(AVCodecContext **decodeCtx,AVStream **stream,AVMediaType type);
    int initDecoder(AVCodecContext **decodeCtx,int *streamIdx,AVMediaType type);
    int initAudioInfo();
    int initVideoInfo();
    int decode(AVCodecContext *decodeCtx,AVPacket *pkt);
    void writeAudioFrame();
    void writeVideoFrame();
};

#endif // DEMUXER_H
