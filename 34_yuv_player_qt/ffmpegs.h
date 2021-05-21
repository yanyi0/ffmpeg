#ifndef FFMPEGS_H
#define FFMPEGS_H
extern "C" {
#include <libavutil/avutil.h>
}

typedef struct {
  char *pixels;
  int width;
  int height;
  AVPixelFormat format;
} RawVideoFrame;

typedef struct {
  const char *filename;
  int width;
  int height;
  AVPixelFormat format;
} RawVideoFile;
class FFmpegs
{
public:
    FFmpegs();
    static void convertVideoRaw(RawVideoFile &in,RawVideoFile &out);
    static void convertVideoFrame(RawVideoFrame &in,RawVideoFrame &out);
};

#endif // FFMPEGS_H
