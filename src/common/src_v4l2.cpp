/*
*
* Copyright ? 2017 NXP
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include "src_v4l2.hpp"

int v4l2Camera::exit_flag = 0; // Exit flag
pthread_mutex_t v4l2Camera::th_mutex = PTHREAD_MUTEX_INITIALIZER;	// Mutex for camera access sinchronization

/**************************************************************************************************************
 *
 * @brief  			v4l2Camera class constructor.
 *
 * @param  in 		int in_width - input frame width
 *					int in_height - input frame height
 *					int in_pixel_fmt - camera pixel format
 *					int in_mem_type - memory type
 *					const char* in_device - camera device name
 *
 * @return 			The function creates the v4l2Camera object.
 *
 * @remarks 		The function creates v4l2Camera object and initializes object attributes.
 *
 **************************************************************************************************************/
v4l2Camera::v4l2Camera(int in_width, int in_height, int in_pixel_fmt, int in_mem_type, const char* in_device)
{
    width = in_width;
    height = in_height;
    pixel_fmt = in_pixel_fmt;
    mem_type = in_mem_type;
    device = string(in_device);

    for (int i = 0; i < BUFFER_NUM; i++)
        memset(&capture_buf[i], 0, sizeof(capture_buf[i]));

    fill_buffer_inx = -1;

    th_arg = (thread_arg){NULL, NULL, NULL, NULL};
}

/**************************************************************************************************************
 *
 * @brief  			v4l2Camera class destructor.
 *
 * @param  in 		-
 *
 * @return 			The function deletes the v4l2Camera object.
 *
 * @remarks 		The function deletes v4l2Camera object and cleans object attributes.
 *
 **************************************************************************************************************/
v4l2Camera::~v4l2Camera()
{

}


/**************************************************************************************************************
 *
 * @brief  			Setup camera capturing
 *
 * @param   		-
 *
 * @return 			The function returns 0 if capturing device was set successfully. Otherwise -1 has been returned.
 *
 * @remarks 		The function opens camera devices and sets capturing mode.
 *
 **************************************************************************************************************/
int v4l2Camera::captureSetup()
{
    struct v4l2_capability cap; // Query device capabilities
    struct v4l2_format fmt; // Data format
    struct v4l2_requestbuffers req; // Parameters of the device buffers
    struct v4l2_streamparm parm; // Streaming parameters
    struct v4l2_fmtdesc fmtdesc; // Format enumeration

    if ((fd = open(device.c_str(), O_RDWR, 0)) < 0)
    {
        cout << "Unable to open " << device << endl;
        return(-1);
    }

    // Identify kernel devices compatible with this specification and to obtain information about driver and hardware capabilities
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        if (EINVAL == errno) {
            cout << device << " is no V4L2 device" << endl;
        }
        else {
            cout << device << " is no V4L device, unknow error" << endl;
        }
        close(fd);
        return(-1);
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) { // The device supports the single-planar API through the Video Capture interface
        cout << device << " is no video capture device" << endl;
        close(fd);
        return(-1);
    }




    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    /* Enum channels fmt */
    for (int i = 0; ; i++) {
        fmtdesc.index = i;
        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) < 0)
        {
            //printf("VIDIOC ENUM FMT failed, index=%d \n", i);
            break;
        }
        //printf("index=%d\n", fmtdesc.index);
        //printf("pixelformat (output by camera): %c%c%c%c\n", fmtdesc.pixelformat & 0xff, (fmtdesc.pixelformat >> 8) & 0xff, (fmtdesc.pixelformat >> 16) & 0xff, (fmtdesc.pixelformat >> 24) & 0xff);
    }


    // Set the data format, try a format
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = pixel_fmt;
    fmt.fmt.pix_mp.num_planes = 1;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) { // VIDIOC_S_FMT may change width and height
        cout << device << " format not supported" << endl;
        close(fd);
        return(-1);
    }


    // Get the data format
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
        cout << "VIDIOC_G_FMT failed" << endl;
        close(fd);
        return(-1);
    }
    else
    {
        cout << "\tWidth = " << fmt.fmt.pix.width << "\t Height = " << fmt.fmt.pix.height << endl;
        cout << "\tImage size = " << fmt.fmt.pix.sizeimage << endl;
        cout << "\tPixelformat " << (char)(fmt.fmt.pix.pixelformat & 0xff) << (char)((fmt.fmt.pix.pixelformat >> 8) & 0xff) <<
                                    (char)((fmt.fmt.pix.pixelformat >> 16) & 0xff) << (char)((fmt.fmt.pix.pixelformat >> 24) & 0xff) << endl;
    }
    width = fmt.fmt.pix.width;
    height = fmt.fmt.pix.height;


    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_G_PARM, &parm) < 0) {
        printf("VIDIOC_G_PARM failed\n");
        parm.parm.capture.timeperframe.denominator = 30;
    }

    printf("\t WxH@fps = %dx%d@%d", fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height, parm.parm.capture.timeperframe.denominator);
    printf("\t Image size = %d\n", fmt.fmt.pix_mp.plane_fmt[0].sizeimage);




    // Initiate Memory Mapping, User Pointer I/O or DMA buffer I/O
    memset(&req, 0, sizeof(req));
    req.count = BUFFER_NUM; // The number of buffers requested or granted
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = mem_type;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        if (EINVAL == errno)
        {
            cout << device << " does not support memory mapping" << endl;
            close(fd);
            return(-1);
        }
        else
        {
            cout << device << " does not support memory mapping, unknow error" << endl;
            close(fd);
            return(-1);
        }
    }

    if (req.count < 2)
    {
        cout << "Insufficient buffer memory on " << device << endl;
        close(fd);
        return(-1);
    }
    return(0);
}

/**************************************************************************************************************
 *
 * @brief  			Start capturing
 *
 * @param   		-
 *
 * @return 			The function returns 0 if capturing was run successfully. Otherwise -1 has been returned.
 *
 * @remarks			The function starts camera capturing
 *
 **************************************************************************************************************/
int v4l2Camera::startCapturing()
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes = { 0 };
    enum v4l2_buf_type type;

    for (int i = 0; i < BUFFER_NUM; i++)
    {
        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof(planes));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = mem_type;
        buf.m.planes = &planes;
        buf.length = 1;
        buf.index = i;

        if (mem_type == V4L2_MEMORY_USERPTR) {
            buf.length =  buffers[i].length;
            buf.m.userptr = (unsigned long) buffers[i].offset;
        }

        // Query the status of a buffer at any time after buffers have been allocated with the VIDIOC_REQBUFS ioctl
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            cout << "VIDIOC_QUERYBUF error" << endl;
            return(-1);
        }
        if (mem_type == V4L2_MEMORY_MMAP) {
            buffers[i].length = buf.m.planes->length;
            buffers[i].offset = (size_t) buf.m.planes->m.mem_offset;
            buffers[i].filled = 0;
            buffers[i].invalid = 0;
            buffers[i].start = (unsigned char*)mmap(NULL, buffers[i].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffers[i].offset);
            memset(buffers[i].start, 0xFF, buffers[i].length);
            //printf("buffer[%d] startAddr=0x%x, offset=0x%x, buf_size=%d\n", i, (unsigned int *)buffers[i].start, buffers[i].offset, buffers[i].length);
        }
    }

    for (int i = 0; i < BUFFER_NUM; i++) {
        memset(&buf, 0, sizeof(buf));
        memset(&planes, 0, sizeof(planes));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = mem_type;
        buf.m.planes = &planes;
        buf.index = i;
        buf.m.planes->length = buffers[i].length;
        buf.length = 1;
        if (mem_type == V4L2_MEMORY_USERPTR)
            buf.m.planes->m.userptr = (unsigned long) buffers[i].start;
        else
            buf.m.planes->m.mem_offset = buffers[i].offset;

        // Enqueue an empty (capturing) or filled (output) buffer in the driver's incoming queue
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            cout << "startVIDIOC_QBUF error" << endl;
            return(-1);
        }
    }

    // Start streaming I/O
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        cout << "VIDIOC_STREAMON error" << endl;
        return(-1);
    }
    return(0);
}

/**************************************************************************************************************
 *
 * @brief  			Stop camera capturing
 *
 * @param   		-
 *
 * @return 			-
 *
 * @remarks			The function stop camera capturing, releases all memory and close camera device.
 *
 **************************************************************************************************************/
void v4l2Camera::stopCapturing()
{
    v4l2Camera::exit_flag = 1;
    void* status = 0;
    if(get_frame_th)
    {
        pthread_join(get_frame_th, &status);
        if (status != 0)
            cout << "Pthread join " << device << " failed" << endl;
    }

    pthread_mutex_unlock(&th_mutex);
    pthread_mutex_destroy(&th_mutex);

    if (fd >= 0)
    {
        //enum v4l2_buf_type type;
        //type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        //if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0)
        //{
        //	cout << "Stop_capturing " << device << " failed" << endl;
        //}
        for (int i = 0; i < BUFFER_NUM; i++)
            munmap(buffers[i].start, buffers[i].length);
        close(fd);
    }
}


/**************************************************************************************************************
 *
 * @brief  			Capturing thread
 *
 * @param   in		void* input_args (thread_arg):	int* fd;	// Thread id
 *													int* fill_buffer_inx;	// Index of buffer which contain last captured camera frame
 *													v4l2_buffer* capture_buf[BUFFER_NUM];	// Pointer to captured buffer
 *													videobuffer* buffers[BUFFER_NUM];		// Pointer to videobuffer structures
 *
 * @return 			-
 *
 * @remarks 		The function creates thread with camera frame capturing loop. The capturing loop is terminated
 *					when the exit flag exit_flag is set to 1. Access to camera is synchronized by the mutex th_mutex.
 *
 **************************************************************************************************************/
void* v4l2Camera::getFrameThread(void* input_args)
{
    struct v4l2_plane planes = { 0 };
    thread_arg th_arg = *((thread_arg *) input_args);
    int i = 0;

    while (!exit_flag)
    {        
        if (*th_arg.fill_buffer_inx != -1)
        {
            i = *th_arg.fill_buffer_inx + 1;
            if (i  == BUFFER_NUM) i = 0;
        }
        else i = 0;
        pthread_mutex_lock(&th_mutex);
        th_arg.buffers[i]->filled = 0;
        th_arg.buffers[i]->invalid = 0;
        pthread_mutex_unlock(&th_mutex);
        if (i != BUFFER_NUM)
        {
            memset(th_arg.capture_buf[i], 0, sizeof(capture_buf[i]));
            memset(&planes, 0, sizeof(planes));
            th_arg.capture_buf[i]->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            th_arg.capture_buf[i]->memory = V4L2_MEMORY_MMAP;
            th_arg.capture_buf[i]->m.planes = &planes;
            th_arg.capture_buf[i]->length = 1;

            if (ioctl(*th_arg.fd, VIDIOC_DQBUF, th_arg.capture_buf[i]) < 0)
            {
                cout << "VIDIOC_DQBUF failed" << endl;
                continue;
            }
            pthread_mutex_lock(&th_mutex);
            th_arg.buffers[i]->filled = 1;
            *th_arg.fill_buffer_inx = i;
            pthread_mutex_unlock(&th_mutex);
        }

        if (ioctl(*th_arg.fd, VIDIOC_QBUF, th_arg.capture_buf[i]) < 0)
        {
            cout << "VIDIOC_QBUF failed" << endl;
            continue;
        }
    }
    pthread_exit((void*)0);
}

/**************************************************************************************************************
 *
 * @brief  			Create thread with camera frame capturing loop
 *
 * @param   		-
 *
 * @return 			The function returns 0 if capturing thread was created successfully.
 *
 * @remarks 		The function creates thread with camera frame capturing loop.
 *
 **************************************************************************************************************/
int v4l2Camera::getFrame()
{
    for (int i = 0; i < BUFFER_NUM; i++)
    {
        th_arg.capture_buf[i] = &capture_buf[i];
        th_arg.buffers[i] = &buffers[i];
    }
    th_arg.fd = &fd;
    th_arg.fill_buffer_inx = &fill_buffer_inx;

    pthread_create(&get_frame_th, NULL, v4l2Camera::getFrameThread, (void *)&th_arg);

    return(0);
}
