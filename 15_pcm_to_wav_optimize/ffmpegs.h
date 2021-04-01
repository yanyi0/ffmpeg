#ifndef FFMPEGS_H
#define FFMPEGS_H
#include <QObject>

typedef struct {
    //RIFFchunkId
    char riffChunkId[4] = {'R','I','F','F'};
    //文件总大小减去8字节
    uint32_t chunkDataSize;
    //fmtchunkId
    uint8_t fmtId[4] = {'W','A','V','E'};
    //subChunkId
    uint8_t fmtChunkId[4] = {'f','m','t',' '};
    //fmtChunkDataSize = 16
    uint32_t fmtChunkDataSize = 16;
    //音频格式 1表示PCM
    uint16_t audioFormat = 1;
    //声道
    uint16_t nubmerChannels;
    //采样率
    uint32_t sampleRate;
    //字节率
    uint32_t byteRate;
    //一个样本所占的字节数
    uint16_t blockAlign;
    //位深度
    uint16_t bitsPerSample;
    //dataChunkId
    uint8_t dataChunkId[4] = {'d','a','d','a'};
    //dataChunkDataSize
    uint32_t dataChunkDataSize;
} WAVHeader;
class FFmpegs
{
public:
    FFmpegs();
    static void pcmToWav(WAVHeader &wavHeader,const char *pcmFileName,const char *wavFileName);
};

#endif // FFMPEGS_H
