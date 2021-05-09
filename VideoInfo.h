#ifndef _VIDEOINFO_
#define _VIDEOINFO_

#include <opencv2/core.hpp>
#include <string>

struct VideoInfo
{
    VideoInfo(std::string* path);
    VideoInfo(int id);
    cv::Size dimensions;
    union Handle
    {
        Handle(std::string* path);
        Handle(int id);
        std::string* path;
        int device_id;
    } v_cap_source;
    double fps;
    int format;
    int codec;
};

#endif
