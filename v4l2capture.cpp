#include "v4l2capture.h"
#include <iostream>
#include <QDebug>

V4l2Capture::V4l2Capture()
{
    init();
}

V4l2Capture::V4l2Capture(std::string &fileName, const ImgFormat &ImgFmt)
{
    init();

    if(open(fileName, ImgFmt)) {
        std::cerr << "Unable to open " << fileName << std::endl;
        if(isOpened())
            std::cout << "Some device is already opened" << std::endl;
        else
            std::cerr << "error: " << errno << std::endl;
    }
}

V4l2Capture::V4l2Capture(int index, const ImgFormat &ImgFmt)
{
    init();

    if(open(index, ImgFmt)) {
        std::cerr << "Unable to open /dev/video" << index << std::endl;
        if(isOpened())
            std::cout << "Some device is already opened" << std::endl;
        else
            std::cerr << "error: " << errno << std::endl;
    }
}

V4l2Capture::~V4l2Capture()
{
    stopCapturing();
    deinitDevice();
    release();
}

int V4l2Capture::open(std::string &fileName, const ImgFormat &ImgFmt)
{
    do {
        if(isOpened()) {
            break;
        }

        fd_cap_v4l = ::open(fileName.c_str(), O_RDWR);
        if(fd_cap_v4l == -1) {
            break;
        }

        if(initDevice(ImgFmt)) {
            break;
        }
        return 0;
    }while(0);

    return -1;
}

int V4l2Capture::open(int index, const ImgFormat &ImgFmt)
{
    std::string name("/dev/video");

    name += std::to_string(index);

    return open(name, ImgFmt);
}

void V4l2Capture::printSupportFormat()
{
    struct v4l2_fmtdesc fmtd;

    memset(&fmtd, 0, sizeof(fmtd));
    fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    while(0 == ioctl(fd_cap_v4l, VIDIOC_ENUM_FMT, &fmtd)) {
        printf("Support format: %c%c%c%c\n", (fmtd.pixelformat >> 0) & 0xff,
                                             (fmtd.pixelformat >> 8) & 0xff,
                                             (fmtd.pixelformat >> 16) & 0xff,
                                             (fmtd.pixelformat >> 24) & 0xff);
        fmtd.index++;
    }
}

bool V4l2Capture::isOpened() const
{
    return (fd_cap_v4l == -1 ? false : true);
}

int V4l2Capture::startCapturing()
{
    enum v4l2_buf_type type;

    for(uint i = 0; i < buffers.size(); i++) {
        v4lBuf.index = i;

        if(ioctl(fd_cap_v4l, VIDIOC_QBUF, &v4lBuf)) {
            printf("error VIDIOC_QBUF, [%d]\n", errno);
            return -1;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if(ioctl(fd_cap_v4l, VIDIOC_STREAMON, &type)) {
        printf("error VIDIOC_STREAMON\n");
        return -1;
    }

    return 0;
}

void V4l2Capture::stopCapturing()
{
    if(fd_cap_v4l != -1) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ioctl(fd_cap_v4l, VIDIOC_STREAMOFF, &type);
    }
}

int V4l2Capture::getFrame(CapBuffers **ppbuf)
{
    if(ioctl(fd_cap_v4l, VIDIOC_DQBUF, &v4lBuf)) {
        printf("get frame error: %d\n", errno);
        return -1;
    }
    *ppbuf = &buffers[v4lBuf.index];
    return 0;
}

int V4l2Capture::doneFrame()
{
    if(ioctl(fd_cap_v4l, VIDIOC_QBUF, &v4lBuf)) {
        printf("done frame error: %d\n", errno);
        return -1;
    }
    return 0;
}

void V4l2Capture::init()
{
    fd_cap_v4l = -1;
    memset(&v4lBuf, 0, sizeof(v4lBuf));
}

int V4l2Capture::initDevice(const ImgFormat &ImgFmt)
{
    struct v4l2_format fmt;
    struct v4l2_requestbuffers rq_buf;

    width = ImgFmt.width;
    height = ImgFmt.height;

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = ImgFmt.width;
    fmt.fmt.pix_mp.height = ImgFmt.height;
    fmt.fmt.pix_mp.pixelformat = ImgFmt.pixFmt;
//    fmt.fmt.pix_mp.field = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix_mp.num_planes = 1;
    if(ioctl(fd_cap_v4l, VIDIOC_S_FMT, &fmt)) {
        printf("VIDIOC_S_FMT error: %d\n", errno);
        goto error;
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if(ioctl(fd_cap_v4l, VIDIOC_G_FMT, &fmt)) {
        printf("VIDIOC_G_FMT error: %d\n", errno);
        goto error;
    }
    printf("Set Current size %d(w) x %d(h); "
           "format: %c%c%c%c\n", fmt.fmt.pix_mp.width,
                                 fmt.fmt.pix_mp.height,
                                 (fmt.fmt.pix_mp.pixelformat >> 0) & 0xff,
                                 (fmt.fmt.pix_mp.pixelformat >> 8) & 0xff,
                                 (fmt.fmt.pix_mp.pixelformat >> 16) & 0xff,
                                 (fmt.fmt.pix_mp.pixelformat >> 24) & 0xff);

    memset(&rq_buf, 0, sizeof(rq_buf));
    rq_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    rq_buf.memory = V4L2_MEMORY_MMAP;
    rq_buf.count = V4L2_BUFFER_NUM;
    if(ioctl(fd_cap_v4l, VIDIOC_REQBUFS, &rq_buf)) {
        printf("this device dose not support mmap\n");
        goto error;
    }

    if (rq_buf.count < V4L2_BUFFER_NUM) {
        printf("not enough buffers, num: %d\n", rq_buf.count);
        goto error_nbuf;
    }
    printf("Setting streaming i/o to mmap buffer; "
           "buffer count: %d\n", rq_buf.count);

    uint n_buffers;
    struct v4l2_plane planes;
    memset(&planes, 0, sizeof(planes));
    v4lBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4lBuf.memory = V4L2_MEMORY_MMAP;
    v4lBuf.m.planes = &planes;
    v4lBuf.length = 1;
    for(n_buffers = 0; n_buffers < rq_buf.count; n_buffers++) {
        CapBuffers buffer;

        v4lBuf.index = n_buffers;
        if(ioctl(fd_cap_v4l, VIDIOC_QUERYBUF, &v4lBuf)) {
            printf("failed VIDIOC_QUERYBUF, error: %d\n", errno);

            if(n_buffers == 0)
                goto error_nbuf;
            else
                goto error_unmap;
        }

        buffer.length = v4lBuf.m.planes->length;
        buffer.offset = (size_t)v4lBuf.m.planes->m.mem_offset;
        buffer.start = mmap(NULL, buffer.length,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd_cap_v4l,
                                buffer.offset);
        if(MAP_FAILED == buffer.start) {
            printf("mmap failed\n");
            goto error_unmap;
        }
        buffers.push_back(buffer);
    }
    return 0;

error_unmap:
    for(uint i = 0; i < buffers.size(); ++i) {
        munmap(buffers[i].start, buffers[i].length);
    }
    buffers.clear();

error_nbuf:
    rq_buf.count = 0;
    ioctl(fd_cap_v4l, VIDIOC_REQBUFS, &rq_buf);

error:
    return -1;
}

void V4l2Capture::deinitDevice()
{
    for(uint i = 0; i < buffers.size(); i++) {
        munmap(buffers[i].start, buffers[i].length);
    }
    buffers.clear();
//    struct v4l2_requestbuffers rq_buf;

//    memset(&rq_buf, 0, sizeof(rq_buf));
//    rq_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
//    rq_buf.memory = V4L2_MEMORY_MMAP;
//    rq_buf.count = 5;
//    int k = ioctl(fd_cap_v4l, VIDIOC_REQBUFS, &rq_buf);
}

void V4l2Capture::release()
{
    if(isOpened()) {
        ::close(fd_cap_v4l);
        fd_cap_v4l = -1;
    }
}

int V4l2Capture::getHeight() const
{
    return height;
}

int V4l2Capture::getWidth() const
{
    return width;
}

