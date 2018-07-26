#ifndef V4L2CAPTURE_H
#define V4L2CAPTURE_H

extern "C"
{
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
}

#include <string>
#include <vector>

#define V4L2_BUFFER_NUM 6

class V4l2Capture
{
public:
    struct CapBuffers {
        void *start;
        size_t offset;
        size_t length;
    };

    enum PixFormat {
        UYVY = V4L2_PIX_FMT_UYVY,
        RGB565 = V4L2_PIX_FMT_RGB565,
        YUYV = V4L2_PIX_FMT_YUYV,
        RGBA = V4L2_PIX_FMT_RGB32,
    };

    struct ImgFormat {
        uint height;
        uint width;
        PixFormat pixFmt;
    };

    V4l2Capture();
    V4l2Capture(std::string &fileName, const ImgFormat &ImgFmt);
    V4l2Capture(int index, const ImgFormat &ImgFmt);
    ~V4l2Capture();

    int open(std::string &fileName, const ImgFormat &ImgFmt);
    int open(int index, const ImgFormat &ImgFmt);
    bool isOpened() const;
    int startCapturing();
    void stopCapturing();
    int getFrame(CapBuffers **ppbuf);
    int doneFrame();
    void printSupportFormat();
    int getWidth() const;
    int getHeight() const;

private:
    void init();
    int initDevice(const ImgFormat &ImgFmt);
    void deinitDevice();
    void release();

    int fd_cap_v4l;
    int width, height;
    std::vector<CapBuffers> buffers;
    struct v4l2_buffer v4lBuf;
};

#endif // V4L2CAPTURE_H
