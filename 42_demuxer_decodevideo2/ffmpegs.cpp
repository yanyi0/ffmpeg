#include "ffmpegs.h"
#include <QDebug>
static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx;
static AVStream *video_stream = NULL, *audio_stream = NULL;
static const char *src_filename = NULL;
static const char *video_dst_filename = NULL;
static const char *audio_dst_filename = NULL;
static FILE *video_dst_file = NULL;
static FILE *audio_dst_file = NULL;
static uint8_t *video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static int video_dst_bufsize;
static uint8_t **audio_dst_data = NULL;
static int       audio_dst_linesize;
static int audio_dst_bufsize;
static int video_stream_idx = -1, audio_stream_idx = -1;
static AVFrame *frame = NULL;
static AVPacket pkt;
static int video_frame_count = 0;
static int audio_frame_count = 0;
static int videoFrameIdx = 0;
static int audioFrameIdx = 0;
//跳跃了多少次空帧呢
static int skipFrameIdx = 0;

//跳跃帧写入了多少帧
static int skipFrameWriteIdx = 0;
//跳跃帧里面的尾部帧，防止开头的跳跃帧被写入
static int isEndFrame = 0;
//正常帧读取后刷新缓冲区刷新了多少帧
static int flushFrameIdx = 0;
//是尾部跳跃帧补齐
static int isSkipFrame = 0;
FFmpegs::FFmpegs()
{

}
static int decode_packet(int *got_frame, int cached)
{
    int ret = 0;
    if (pkt.stream_index == video_stream_idx) {
        /* decode video frame */
        ret = avcodec_decode_video2(video_dec_ctx, frame, got_frame, &pkt);
        if(isEndFrame == 1) qDebug() << "尾部帧刷新后解码器返回值:" << ret << *got_frame << &pkt;
        if (ret < 0) {
            fprintf(stderr, "Error decoding video frame\n");
            return ret;
        }
        if (*got_frame) {
            qDebug() << "正常视频帧:" << ++video_frame_count << "帧宽度:" << frame->width;
            if(isEndFrame == 1){//如果是尾部刷新缓冲区记录下刷新缓冲区刷新了多少帧
                flushFrameIdx++;
                qDebug() << "缓冲区刷新了多少帧" << flushFrameIdx;
            }
            printf("视频帧-------video_frame%s n:%d coded_n:%d pts:%s\n",
                   cached ? "(cached)" : "",
                   video_frame_count, frame->coded_picture_number,
                   av_ts2timestr(frame->pts, &video_dec_ctx->time_base));
            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
            av_image_copy(video_dst_data, video_dst_linesize,
                          (const uint8_t **)(frame->data), frame->linesize,
                          video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height);
            /* write to rawvideo file */
            fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
        }else{
            if(isEndFrame==0){//不是尾部刷新缓冲区进入才计算跳跃次数
                skipFrameIdx++;
                qDebug() << "当前pkt的值" << &pkt << "帧宽度:" << frame->width << "--跳跃次数--" << skipFrameIdx;
            }
            else{//是尾部跳跃帧数进来，将尾部帧写入文件尾部
                if(isSkipFrame == 1){
                    qDebug() << "got_frame写入帧为0，写入跳跃帧次数" << ++skipFrameWriteIdx;
                    av_image_copy(video_dst_data, video_dst_linesize,
                                  (const uint8_t **)(frame->data), frame->linesize,
                                  video_dec_ctx->pix_fmt, video_dec_ctx->width, video_dec_ctx->height);
                    fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
                }
            }
        }
    }
    else if (pkt.stream_index == audio_stream_idx) {
        /* decode audio frame */
        ret = avcodec_decode_audio4(audio_dec_ctx, frame, got_frame, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error decoding audio frame\n");
            return ret;
        }
        if (*got_frame) {
            printf("音频帧--------audio_frame%s n:%d nb_samples:%d pts:%s\n",
                   cached ? "(cached)" : "",
                   audio_frame_count++, frame->nb_samples,
                   av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));
            ret = av_samples_alloc(audio_dst_data, &audio_dst_linesize, av_frame_get_channels(frame),
                                   frame->nb_samples, (AVSampleFormat)frame->format, 1);
            if (ret < 0) {
                fprintf(stderr, "Could not allocate audio buffer\n");
                return AVERROR(ENOMEM);
            }
            /* TODO: extend return code of the av_samples_* functions so that this call is not needed */
            audio_dst_bufsize =
                av_samples_get_buffer_size(NULL, av_frame_get_channels(frame),
                                           frame->nb_samples, (AVSampleFormat)frame->format, 1);
            /* copy audio data to destination buffer:
             * this is required since rawaudio expects non aligned data */
            av_samples_copy(audio_dst_data, frame->data, 0, 0,
                            frame->nb_samples, av_frame_get_channels(frame), (AVSampleFormat)frame->format);
            /* write to rawaudio file */
            printf("单次写入音频帧的大小 %d\n写了多少次 %d",audio_dst_bufsize,++audioFrameIdx);
            fwrite(audio_dst_data[0], 1, audio_dst_bufsize, audio_dst_file);
            av_freep(&audio_dst_data[0]);
        }
    }
    return ret;
}
static int open_codec_context(int *stream_idx,
                              AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret;
    AVStream *st;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *dec = NULL;
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), src_filename);
        return ret;
    } else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx];
        /* find decoder for the stream */
        dec_ctx = st->codec;
        dec = avcodec_find_decoder(dec_ctx->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
    }
    return 0;
}
static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = NULL;
    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }
    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}
void FFmpegs::demuxer(){
    int ret = 0, got_frame;
    int inEnd = 0;
    src_filename = "/Users/cloud/Documents/iOS/音视频/TestMusic/Demux/in.mp4";
    video_dst_filename = "/Users/cloud/Documents/iOS/音视频/TestMusic/Demux/out_video2_optimize.yuv";
    audio_dst_filename = "/Users/cloud/Documents/iOS/音视频/TestMusic/Demux/out_video2_optimize.pcm";
    /* register all formats and codecs */
    av_register_all();
    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, src_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", src_filename);
        exit(1);
    }
    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }
    if (open_codec_context(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];
        video_dec_ctx = video_stream->codec;
        video_dst_file = fopen(video_dst_filename, "wb");
        if (!video_dst_file) {
            fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
            ret = 1;
            goto end;
        }
        /* allocate image where the decoded image will be put */
        ret = av_image_alloc(video_dst_data, video_dst_linesize,
                             video_dec_ctx->width, video_dec_ctx->height,
                             video_dec_ctx->pix_fmt, 1);
        if (ret < 0) {
            fprintf(stderr, "Could not allocate raw video buffer\n");
            goto end;
        }
        video_dst_bufsize = ret;
    }
    if (open_codec_context(&audio_stream_idx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        int nb_planes;
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dec_ctx = audio_stream->codec;
        audio_dst_file = fopen(audio_dst_filename, "wb");
        if (!audio_dst_file) {
            fprintf(stderr, "Could not open destination file %s\n", video_dst_filename);
            ret = 1;
            goto end;
        }
        nb_planes = av_sample_fmt_is_planar(audio_dec_ctx->sample_fmt) ?
            audio_dec_ctx->channels : 1;
        audio_dst_data = (uint8_t **)av_mallocz(sizeof(uint8_t *) * nb_planes);
        if (!audio_dst_data) {
            fprintf(stderr, "Could not allocate audio data buffers\n");
            ret = AVERROR(ENOMEM);
            goto end;
        }
    }
    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);
    if (!audio_stream && !video_stream) {
        fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        goto end;
    }
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }
    /* initialize packet, set data to NULL, let the demuxer fill it */
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    if (video_stream)
        printf("Demuxing video from file '%s' into '%s'\n", src_filename, video_dst_filename);
    if (audio_stream)
        printf("Demuxing audio from file '%s' into '%s'\n", src_filename, audio_dst_filename);
    /* read frames from the file */
    //跳跃帧次数+正常帧+缓冲区帧数>=总帧数
    //正常帧
    while (av_read_frame(fmt_ctx, &pkt) >= 0) {
        decode_packet(&got_frame, 0);
        av_free_packet(&pkt);
    }
    /* flush cached frames */
    pkt.data = NULL;
    pkt.size = 0;
    //刷新缓冲区帧
    do {
        qDebug() << "进入刷新缓冲区空帧读取" << got_frame;
        isEndFrame = 1;
        decode_packet(&got_frame, 1);
    } while (got_frame);
    //跳跃帧
    while(skipFrameIdx--){
        isSkipFrame = 1;
        qDebug() << "进入跳跃帧读取" << skipFrameIdx;
        decode_packet(&got_frame, 1);
    }
    printf("Demuxing succeeded.\n");
    if (video_stream) {
        printf("Play the output video file with the command:\n"
               "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
               av_get_pix_fmt_name(video_dec_ctx->pix_fmt), video_dec_ctx->width, video_dec_ctx->height,
               video_dst_filename);
    }
    if (audio_stream) {
        const char *fmt;
        if ((ret = get_format_from_sample_fmt(&fmt, audio_dec_ctx->sample_fmt)) < 0)
            goto end;
        printf("Play the output audio file with the command:\n"
               "ffplay -f %s -ac %d -ar %d %s\n",
               fmt, audio_dec_ctx->channels, audio_dec_ctx->sample_rate,
               audio_dst_filename);
    }
end:
    if (video_dec_ctx)
        avcodec_close(video_dec_ctx);
    if (audio_dec_ctx)
        avcodec_close(audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    if (video_dst_file)
        fclose(video_dst_file);
    if (audio_dst_file)
        fclose(audio_dst_file);
    av_free(frame);
    av_free(video_dst_data[0]);
    av_free(audio_dst_data);
}
