#include "ffmpegs.h"
#include <QDebug>
#include <QFile>
extern "C" {
  #include <libswscale/swscale.h>
  #include <libavutil/opt.h>
  #include <libavutil/imgutils.h>
}
#define ERROR_BUF  \
        char errbuf[1024]; \
        av_strerror(ret,errbuf,sizeof(errbuf));
#define END(func) \
    if(ret < 0){ \
        ERROR_BUF; \
        qDebug() << #func << "error" << errbuf;\
        goto end;\
    }
FFmpegs::FFmpegs()
{

}
void FFmpegs::convertVideoFrame(RawVideoFrame &in,RawVideoFrame &out){
    //上下文
    SwsContext *ctx = nullptr;
    //指针数组的大小为4，YUV或RGB，加一个透明度A
    //输入、输出缓冲区(指向每一个平面的数据)
    uint8_t *inData[4],*outData[4];
    int inStrides[4],outStrides[4];
    //每一帧图片的大小
    int inFrameSize,outFrameSize;
    //返回值
    int ret = 0;
    //输入缓冲区
    ret = av_image_alloc(inData,inStrides,in.width,in.height,in.format,1);
    END(av_image_alloc);
    //输出缓冲区
    ret = av_image_alloc(outData,outStrides,out.width,out.height,out.format,1);
    END(av_image_alloc);
    //创建上下文
    ctx = sws_getContext(in.width,in.height,in.format,
                         out.width,out.height,out.format,
                         SWS_BILINEAR,nullptr,nullptr,nullptr);
    if(!ctx){
        qDebug() << "sws_getContext error";
        goto end;
    }
    //计算每一帧图片的大小
    inFrameSize = av_image_get_buffer_size(in.format,in.width,in.height,1);
    outFrameSize = av_image_get_buffer_size(out.format,out.width,out.height,1);
    //拷贝一帧数据到inData[0]开始的位置
    memcpy(inData[0],in.pixels,inFrameSize);
    //转换逻辑
    sws_scale(ctx,inData,inStrides,0,in.height,outData,outStrides);
    //从outData[0]写入一帧数据输出out.pixels中
    out.pixels = (char *)malloc(outFrameSize);
    memcpy(out.pixels,outData[0],outFrameSize);
end:
    av_freep(&inData[0]);
    av_freep(&outData[0]);
    sws_freeContext(ctx);
}
/*
 * 自己组装inData
//yuv420p:一帧图片的摆放是先放所有的Y,再放所有的U,再放所有的V,如YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYUUUUUUUUUUUUUUUVVVVVVVVVVVVV
//一帧yuv420p的大小
inData[0] = (uint8_t *)malloc(in.frameSize);
inData[1] = inData[0] + 所有Y的大小
inData[2] = inData[0] + 所有Y的大小 + 所有U的大小
//拷贝像素数据到inData[0]指向的堆空间,覆盖掉一整帧的数据，本身一帧yuv420p的大小就是inData[0]指向的开辟的空间的大小
 格式也为YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYUUUUUUUUUUUUUUUVVVVVVVVVVVVV正好能覆盖到Y是Y,U是U，V是V
inStrides[0] = in.width * in.height * 1; 在一帧yuv图片中每个平面的宽度比如Y的宽度为有多少个Y,多少个像素
inStrides[1] = inStrides[0] >> 2;
inStrides[2] = inStrides[1];
*/
void FFmpegs::convertVideoRaw(RawVideoFile &in,RawVideoFile &out){

    //上下文
    SwsContext *ctx = nullptr;
    //指针数组的大小为4，YUV或RGB，加一个透明度A
    //输入、输出缓冲区(指向每一个平面的数据)
    uint8_t *inData[4],*outData[4];
    int inStrides[4],outStrides[4];
    //每一帧图片的大小
    int inFrameSize,outFrameSize;
    //返回值
    int ret = 0;
    //进行到了哪一帧
    int inxFrame = 0;
    QFile inFile(in.filename);
    QFile outFile(out.filename);
    //输入缓冲区
    ret = av_image_alloc(inData,inStrides,in.width,in.height,in.format,1);
    END(av_image_alloc);
    //输出缓冲区
    ret = av_image_alloc(outData,outStrides,out.width,out.height,out.format,1);
    END(av_image_alloc);
    //创建上下文
    ctx = sws_getContext(in.width,in.height,in.format,
                         out.width,out.height,out.format,
                         SWS_BILINEAR,nullptr,nullptr,nullptr);
    if(!ctx){
        qDebug() << "sws_getContext error";
        goto end;
    }
    //打开文件
    if(!inFile.open(QFile::ReadOnly)){
        qDebug() << "file open error" << in.filename;
        goto end;
    }
    if(!outFile.open(QFile::WriteOnly)){
        qDebug() << "file open error" << out.filename;
        goto end;
    }
    //计算每一帧图片的大小
    inFrameSize = av_image_get_buffer_size(in.format,in.width,in.height,1);
    outFrameSize = av_image_get_buffer_size(out.format,out.width,out.height,1);
    while (inFile.read((char *)inData[0],inFrameSize) == inFrameSize) {
        //转换逻辑
        sws_scale(ctx,inData,inStrides,0,in.height,outData,outStrides);
        //写入输出文件
        outFile.write((char *)outData[0],outFrameSize);
        //转换到了哪一帧
        qDebug() << "转换到了第" << inxFrame++ << "帧";
    }

end:
    inFile.close();
    outFile.close();
    av_freep(&inData[0]);
    av_freep(&outData[0]);
    sws_freeContext(ctx);
}
