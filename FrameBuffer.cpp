#include "FrameBuffer.h"

FrameBuffer::FrameBuffer() : buff_size(0) {};

FrameBuffer::FrameBuffer(size_t buff_size) : buff_size(buff_size)
{
    if(buff_size) return;

    buff = new cv::Mat[buff_size];
}

FrameBuffer::~FrameBuffer() { delete[] buff; }

void FrameBuffer::add_frame(cv::Mat& frame)
{
    if(!buff_size) return;

    newest = (newest+1)%buff_size;
    frame.copyTo(buff[newest]);
    
    if(frame_cnt < buff_size)
        frame_cnt++;
}

void FrameBuffer::write_frames(cv::VideoWriter& video)
{
    if(!buff_size) return;

    for(size_t i=(buff_size+newest)-(frame_cnt-1); i<buff_size+newest; i++)
        video << buff[i%buff_size];
    frame_cnt = 0;
    newest = buff_size;
}
