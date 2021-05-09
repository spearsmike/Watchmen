#include "VideoInfo.h"

VideoInfo::VideoInfo(std::string* path) : v_cap_source(path) {};

VideoInfo::VideoInfo(int id) : v_cap_source(id) {};

VideoInfo::Handle::Handle(std::string* path) : path(path) {};

VideoInfo::Handle::Handle(int id) : device_id(id) {};
