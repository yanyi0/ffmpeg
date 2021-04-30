#include "ffmpegs.h"
#include "QDebug"
FFmpegs::FFmpegs()
{

}
void FFmpegs:: resampleAudio(ResampleAudioSpec &in, ResampleAudioSpec &out){
    resampleAudio(in.filename,in.sampleRate,in.sampleFmt,in.chLayout,
                  out.filename,out.sampleRate,out.sampleFmt,out.chLayout
                  );
}
void FFmpegs:: resampleAudio(const char * inFilename,
                          int inSampleRate,
                          AVSampleFormat inSampleFmt,
                          int inChLayout,
                          const char * outFilename,
                          int outSampleRate,
                          AVSampleFormat outSampleFmt,
                          int outChLayout
                          ){

    //文件名
    QFile inFile(inFilename);
    QFile outFile(outFilename);

    qDebug() << "av_sample_fmt_is_planar" << av_sample_fmt_is_planar(inSampleFmt);
    //av_sample_fmt_is_planar 0

    //开辟缓冲区源码分析
    /*
     *
     * int av_samples_alloc_array_and_samples(uint8_t ***audio_data, int *linesize, int nb_channels,
                                        int nb_samples, enum AVSampleFormat sample_fmt, int align)
       {
          int ret, nb_planes = av_sample_fmt_is_planar(sample_fmt) ? nb_channels : 1;
          *audio_data = av_calloc(nb_planes, sizeof(**audio_data));
          if (!*audio_data)
          return AVERROR(ENOMEM);
          ret = av_samples_alloc(*audio_data, linesize, nb_channels,
                            nb_samples, sample_fmt, align);
          if (ret < 0)
         av_freep(audio_data);
         return ret;
        }
        查看av_calloc源码开辟的内存空间大小为nb_planes * sizeof(**audio_data),nb_planes的值由av_sample_fmt_is_planar(sample_fmt),此刻不是planar,则nb_planes = 1
        开辟的内存空间的大小则为sizeof(**audio_data),开辟的是**audio_data这个指针指向的一片uint_8 *区域
        inData = av_calloc(nb_planes, sizeof(uint8_t *));

        void *av_mallocz(size_t size)
        {
          void *ptr = av_malloc(size);
          if (ptr)
          memset(ptr, 0, size);
          return ptr;
        }
        void *av_calloc(size_t nmemb, size_t size)
        {
          size_t result;
          if (av_size_mult(nmemb, size, &result) < 0)//返回nmemb*size赋值给result
          return NULL;
          return av_mallocz(result);
        }
        static inline int av_size_mult(size_t a, size_t b, size_t *r)
        {
         size_t t = a * b;
         if ((a | b) >= ((size_t)1 << (sizeof(size_t) * 4)) && a && t / a != b)
         return AVERROR(EINVAL);
         *r = t;
         return 0;
        }
     */

    //创建输入缓冲区
    //指向缓冲区的指针
    uint8_t **inData = nullptr;
    //缓冲区的大小
    int inLinesize = 0;
    //声道数
    int inChs = av_get_channel_layout_nb_channels(inChLayout);
    //每个输入样本的大小
    int inBytesPerSample = inChs * av_get_bytes_per_sample(inSampleFmt);
    //缓冲区的样本数量
    int inSamples = 1024;
    //读取文件数据的大小
    int len = 0;

    //创建输出缓冲区
    //指向缓冲区的指针
    uint8_t **outData = nullptr;
    //缓冲区的大小
    int outLinesize = 0;
    //声道数
    int outChs = av_get_channel_layout_nb_channels(outChLayout);
    //每个输入样本的大小
    int outBytesPerSample = outChs * av_get_bytes_per_sample(outSampleFmt);
    //缓冲区的样本数量
 //   int outSamples = 1024;

    int outSamples = av_rescale_rnd(outSampleRate,inSamples,inSampleRate,AV_ROUND_UP);

    qDebug() << "输入缓冲区" << inSampleRate << inSamples;
    qDebug() << "输出缓冲区" << outSampleRate << outSamples;

    //由结果得知44100_2_f32le转48000_1_s16le，数据会有缺省，代码转的比adobeaudition软件和官方ffmpeg命令行转的小一点
    //那么问题出在哪儿了呢？
    //问题出在同样是一秒钟，样本数量发生了变化，之前一秒钟是采集44100个样本，要转化的样本数量为48000,多出了样本数量，而缓冲区的样本数量是相等的设置为1024
    /*
     * inSampleRate      inSamples           44100       1024
     * ------------   =  ----------   ==>   --------  = ------
     * outSampleRate     outSamples          48000        ?
     * 向上取整:后面有小数位的都变为零，向小数点前面的位进1
     * 向下取整:后面有小数位的都直接变为零，不进位1
     *
     * 此时算出来的13004480字节，和命令行转换的13004548字节仍旧有差距，差了68个字节，因为输出缓冲区最后还有一些残留的样本没有刷出去到文件中
     */
    //向下取整 AV_ROUND_DOWN(2.666) = 2
    qDebug() << av_rescale_rnd(8,1,3,AV_ROUND_DOWN);
    //向上取整  AV_ROUND_UP(1.25) = 2
    qDebug() << av_rescale_rnd(5,1,4,AV_ROUND_UP);
    //返回结果
    int ret = 0;
    //创建重采样上下文
    SwrContext *ctx = swr_alloc_set_opts(nullptr,
                                         //输出参数
                                         outChLayout,outSampleFmt,outSampleRate,
                                         //输入参数
                                         inChLayout,inSampleFmt,inSampleRate,
                                         0,nullptr);
    if(!ctx){
        qDebug() << "swr_alloc_set_opts";
        return;
    }

    //初始化重采样上下文
    ret = swr_init(ctx);
    if(ret < 0){
        //根据函数返回的错误码获取错误信息
        AV_ERROR(ret);
        qDebug() << "swr_init error" << errorbuf;
        goto end;
    }
    //创建输入缓冲区
    ret = av_samples_alloc_array_and_samples(&inData,&inLinesize,inChs,inSamples,inSampleFmt,1);
    if(ret < 0){
        AV_ERROR(ret);
        qDebug() << "av_samples_alloc_array_and_samples error" << errorbuf;
        goto end;
    }
    //创建输出缓冲区
    ret = av_samples_alloc_array_and_samples(&outData,&outLinesize,outChs,outSamples,outSampleFmt,1);
    if(ret < 0){
        AV_ERROR(ret);
        qDebug() << "av_samples_alloc_array_and_samples error" << errorbuf;
        goto end;
    }

    //打开文件
    if(!inFile.open(QFile::ReadOnly)){
        qDebug() << "inFile open error" << inFilename;
        goto end;
    }
    if(!outFile.open(QFile::WriteOnly)){
        qDebug() << "outFile open error" << outFilename;
        goto end;
    }

    //读取文件数据 inData[0] = *inData   <==> *(p+i) = p[i]  ==> *p = p[0]
    while((len = inFile.read((char *) inData[0],inLinesize)) > 0){
        //读取的输入缓冲区的样本大小
        inSamples = len / inBytesPerSample;
        //重采样(返回值为重采样后的样本数量)
        ret = swr_convert(ctx,outData,outSamples,(const uint8_t **)inData,inSamples);
        qDebug() << "重采样后的样本数量" << ret;
        if(ret < 0){
            AV_ERROR(ret);
            qDebug() << "swr_convert error" << errorbuf;
            goto end;
        }
        //将转换后的数据写入到输出文件中
        outFile.write((char *)outData[0],ret*outBytesPerSample);
    }
    //检查一下输出缓冲区是否还有残留的样本(已经重采样过的，转换过的)
    //outData[0] = *outData
    while((ret = swr_convert(ctx,outData,outSamples,nullptr,0)) > 0){
        qDebug() << "输出缓冲区残留的样本数量" << ret << "残留的字节大小" << ret*outBytesPerSample;

 //       int size = av_samples_get_buffer_size(nullptr,outChs,ret,outSampleFmt,1);
 //       outFile.write((char *)outData[0],size);
        outFile.write((char *)outData[0],ret*outBytesPerSample);
    }

    //此时和官方的一样大小13004548字节
    /*计算多少个样本占用多少字节
    **Get the required buffer size for the given audio parameters.
    *
    * @param[out] linesize calculated linesize, may be NULL
    * @param nb_channels   the number of channels
    * @param nb_samples    the number of samples in a single channel
    * @param sample_fmt    the sample format
    * @param align         buffer size alignment (0 = default, 1 = no alignment)
    * @return              required buffer size, or negative error code on failure
    * av_samples_get_buffer_size()
    */

 end:
    //释放资源
    inFile.close();
    outFile.close();

    //一共存在4块堆空间，inData[0]指向的存放pcm原始数据的存放unit8_t的内存空间
    //inData指向的存放uint8_t *存放pcm原始数据地址的内存空间
    //同理outData指向另外两块内存空间
    //下面释放4块堆空间,传入地址方便将指针值为nullptr，若传入变量，只能改变形参变量的值，并不能将指针值为nullptr
    //释放输入缓冲区
    if(inData){
        av_freep(&inData[0]);
    }
    av_freep(&inData);
    //释放输出缓冲区
    if(outData){
        av_freep(&outData[0]);
    }
    av_freep(&outData);
    //释放重采样上下文
    swr_free(&ctx);
}
