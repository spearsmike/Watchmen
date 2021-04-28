#include <opencv2/videoio.hpp>  // cv::VideoCapture cv::VideoWriter
#include <opencv2/imgproc.hpp>  // cv::findContours cv::mean cv::erode cv::cvtColor cv::absdiff cv::threshold cv::bitwise_and
#ifdef TEST
#include <opencv2/highgui.hpp>  // cv::imshow cv::createTrackbar
#include <opencv2/viz.hpp>      // cv::viz::Color
#endif
#include <iostream>             // std::cout

// Basic Global Thresholding
inline ushort find_threshold(cv::Mat& img, double stop_threshold)
{
    ushort T = 256/2;
    double dt = std::numeric_limits<double>::max();
    std::vector<uchar> G1, G2;
    int it=0;
    while(dt > stop_threshold)
    {
        for(int x=0; x<img.rows; x++)
        {
            for(int y=0; y<img.cols; y++)
            {
                uchar pixel = img.at<uchar>(x,y);
                if(pixel > T)
                    G1.push_back(pixel);
                else
                    G2.push_back(pixel);
            }
        }
        dt = abs(T-1/2.0*(cv::mean(G1)[0] + cv::mean(G2)[0]));
        T = 1/2.0*(cv::mean(G1)[0] + cv::mean(G2)[0]);
        it++;
    }
    //std::cout << "iterations=" << it << "\tT=" << T << '\n';
    return T;
}

int main(int argc, char* argv[])
{
    struct Buff
    {
        Buff() { buff = new cv::Mat[buff_size]; }
        Buff(size_t buff_size) : buff_size(buff_size) { buff = new cv::Mat[buff_size]; }
        ~Buff() { delete[] buff; }
        void add_frame(cv::Mat& frame)
        {
            newest = (newest+1)%buff_size;
            frame.copyTo(buff[newest]);
            
            if(frame_cnt < buff_size)
                frame_cnt++;
        }
        void write_frames(cv::VideoWriter& video)
        {
            for(size_t i=(buff_size+newest)-(frame_cnt-1); i<buff_size+newest; i++)
                video << buff[i%buff_size];
            frame_cnt = 0;
            newest = 0;
        }
        cv::Mat* buff;
        size_t buff_size = 30;
        size_t frame_cnt = 0;
        size_t newest = buff_size;
    };

    struct
    {
        cv::Size dimensions;
        union {char* path; int device_id=0;} v_cap_source;
        double fps;
        int format;
        int codec;
    } capture_info;

#ifdef TEST
    if(argc < 2)
    {
        std::cout << "Provide a path to the test video\n";
        return 0;
    }
    else
    {
        capture_info.v_cap_source.path = argv[1];
    }
#else
    if(argc >= 2)
        capture_info.v_cap_source.device_id = atoi(argv[1]);
#endif
    // the min diff amount counted as motion, faster motion creates a larger absolute diff
    // it should be big enough to catch motion but small enough to filter out noise
    int motion_threshold = 7;

    // the minimum ratio of different pixels in a frame that count as motion
    float motion_sensitivity = 0.02;

    // keeps track of the number of frames without motion
    int frames_no_motion = std::numeric_limits<int>::max();

    // whether there was motion in the frame
    bool is_motion = false;

    // whether a new video should be created.
    bool new_video = true;

    // erosion kernel size
    int k_size = 2;

#ifdef TEST
    cv::VideoCapture cap(capture_info.v_cap_source.path);
#else
    cv::VideoCapture cap(capture_info.v_cap_source.device_id);
#endif
    capture_info.dimensions = cv::Size(cap.get(cv::CAP_PROP_FRAME_WIDTH), cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    capture_info.format = cap.get(cv::CAP_PROP_FORMAT);
    capture_info.fps = cap.get(cv::CAP_PROP_FPS);
    capture_info.codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    // the maximum number fo frames without motion before video capture is stopped
    size_t max_no_motion = 10;
    if(argc>=3)
        max_no_motion = static_cast<size_t>(atof(argv[2])*capture_info.fps);

    Buff prefix_buffer(max_no_motion);
    cv::VideoWriter outputVid;
    cv::Mat curr_frame_color;

    cv::Mat curr_frame(capture_info.dimensions, CV_MAKETYPE(capture_info.format, 1));
    cv::Mat prev_frame = curr_frame.clone();
    cv::Mat prev_prev_frame = prev_frame.clone();

#ifdef TEST
    int sensitivity_slider = 7;
    cv::namedWindow("Current frame");
    cv::createTrackbar("sensitivity", "Current frame", &sensitivity_slider, 100);
    cv::createTrackbar("pixel difference threshold", "Current frame", &motion_threshold, 50);
    cv::createTrackbar("erosion kernel size", "Current frame", &k_size, 10);
#endif

    cv::Mat diff1, diff2, motion;
    std::vector<std::vector<cv::Point>> contours;
    int frame_count = 0;
    while(1)
    {
        cap >> curr_frame_color;
        // make sure the frame isn't empty (end of video if testing)
#ifdef TEST
        if(curr_frame_color.empty())
        {
            std::cout << "End of video\n";
            break;
            cap.open(capture_info.v_cap_source.path);
            cap >> curr_frame_color;
        }
#endif
        prefix_buffer.add_frame(curr_frame_color);

        // converting to BW makes countNonZero possible
        cv::cvtColor(curr_frame_color, curr_frame, cv::COLOR_BGR2GRAY);
        // take two diffs between three frames
        cv::absdiff(curr_frame, prev_frame, diff1);
        cv::absdiff(prev_frame, prev_prev_frame, diff2);
        cv::threshold(diff1, diff1, motion_threshold, 255, cv::THRESH_BINARY);
        cv::threshold(diff2, diff2, motion_threshold, 255, cv::THRESH_BINARY);
        // remove small one frame movements and noise
        cv::bitwise_and(diff1, diff2, motion);

        cv::erode(motion, motion, cv::getStructuringElement(cv::MorphShapes::MORPH_RECT, cv::Size(k_size, k_size)));
#ifdef TEST
        is_motion = static_cast<float>(cv::countNonZero(motion))/capture_info.dimensions.area() > sensitivity_slider/10000.0;
#else
        is_motion = static_cast<float>(cv::countNonZero(motion))/capture_info.dimensions.area() > motion_sensitivity;
#endif
        if(is_motion)
        {
            frames_no_motion = 0;
#ifdef TEST
            cv::putText(curr_frame_color, "MOTION", cv::Point(10,50), cv::FONT_HERSHEY_PLAIN, 3, cv::viz::Color::green());
            
            // create a bounding box around each contour
            cv::findContours(motion, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            for(auto contour : contours)
                cv::rectangle(curr_frame_color, cv::boundingRect(contour), cv::viz::Color::magenta(), 3);
#endif
        }
        else
        {
            frames_no_motion++;
        }
        // if there was motion in the last <max_no_motion> frames save the current frame
        if(frames_no_motion > max_no_motion)
        {
            new_video = true;
        } else
        {
            if(new_video)
            {
                std::string name = "OUT_Frame="+std::to_string(frame_count)+".mp4";
                outputVid.open(name, capture_info.codec, capture_info.fps, capture_info.dimensions);
                std::cout << "Created " << name << '\n';
                // save previous frames
                prefix_buffer.write_frames(outputVid);
                new_video = false;
            }
            outputVid << curr_frame_color;
        }
#ifdef TEST
        cv::imshow("Current frame", curr_frame_color);
        cv::imshow("AND", motion);
        cv::waitKey(1000/capture_info.fps);
#endif
        prev_prev_frame = prev_frame.clone();
        prev_frame = curr_frame.clone();
        frame_count++;
    }

    cap.release();
    outputVid.release();
#ifdef TEST
    cv::destroyAllWindows();
#endif

    return 0;
}