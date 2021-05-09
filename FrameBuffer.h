#ifndef _FRAMEBUFFER_
#define _FRAMEBUFFER_

#include <opencv2/videoio.hpp>  // cv::VideoCapture cv::VideoWriter

struct FrameBuffer
{
    FrameBuffer();
    FrameBuffer(size_t buff_size);
    ~FrameBuffer();
    void add_frame(cv::Mat& frame);
    void write_frames(cv::VideoWriter& video);
    const size_t buff_size;
    private:
    cv::Mat* buff;
    size_t frame_cnt = 0;
    size_t newest = buff_size;
};

#endif
