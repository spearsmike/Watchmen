#include <opencv2/videoio.hpp>  // cv::VideoCapture cv::VideoWriter
#include <opencv2/imgproc.hpp>  // cv::findContours cv::mean cv::erode cv::cvtColor cv::absdiff cv::threshold cv::bitwise_and
#include <opencv2/highgui.hpp>  // cv::imshow cv::createTrackbar
#include <opencv2/viz.hpp>      // cv::viz::Color
#include <iostream>             // std::cout

#include "VideoInfo.h"
#include "FrameBuffer.h"

// Basic Global Thresholding
inline ushort find_threshold(cv::Mat& img, double stop_threshold)
{
    ushort T = 256/2;
    double dt = std::numeric_limits<double>::max();
    std::vector<uchar> G1, G2;
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
    }
    return T;
}

int main(int argc, char* argv[])
{
    const std::string keys =
    "{h help           |          | Print this message                           }"
    "{v view           |          | View the video as it's processed             }"
    "{i inputvideo     |          | Whether input is a prerecorded video         }"
    "{pre              | 1        | Seconds of video to prepend                  }"
    "{post             | 1        | Seconds of video to postpend                 }"
    "{@input           | 0        | Device id / Path to video                    }"
    "{@output          | ~/Videos | Output video path                            }"
    "{@threshold       | 7        | The pixel difference counted as motion       }"
    "{@sensitivity     | 0.02     | The proportion of the image that's different }"
    ;
    cv::CommandLineParser parser(argc, argv, keys);
    parser.about("Version info");

    if(parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }

    const bool view = parser.has("view");
    const bool input_vid = parser.has("inputvideo");

    // a struct to amalgamate various video capture info
    struct VideoInfo* input_vid_info;

    if(input_vid)
        input_vid_info = new struct VideoInfo(new std::string(parser.get<std::string>(0)));
    else
        input_vid_info = new struct VideoInfo(parser.get<int>(0));

    std::string output_path = parser.get<std::string>(1);

    // the min diff amount counted as motion, faster motion creates a larger absolute diff
    // it should be big enough to catch motion but small enough to filter out noise
    int motion_threshold = parser.get<int>(2);

    // the minimum ratio of different pixels in a frame that count as motion
    float motion_sensitivity = parser.get<float>(3);

    // Seconds of video to prepend
    const float s_pre = parser.get<float>("pre");

    // Seconds of video to postpend
    // >= pre
    const float s_post = parser.get<float>("post");

    // all command line arguments have been saved, check for errors
    if(!parser.check())
    {
        parser.printErrors();
        std::cout << "Use -h --help for program usage\n";
        return 0;
    }

    // keeps track of the number of frames without motion
    int frames_no_motion = std::numeric_limits<int>::max();

    // whether there was motion in the frame
    bool is_motion = false;

    // whether a new video should be created.
    bool new_video = true;

    // erosion kernel size
    int k_size = 2;

    int sensitivity_slider = motion_threshold;

    cv::VideoCapture cap;
    if(input_vid)
        cap = cv::VideoCapture(*(input_vid_info->v_cap_source.path));
    else
        cap = cv::VideoCapture(input_vid_info->v_cap_source.device_id);

    input_vid_info->dimensions = cv::Size(cap.get(cv::CAP_PROP_FRAME_WIDTH), cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    input_vid_info->format = cap.get(cv::CAP_PROP_FORMAT);
    input_vid_info->fps = cap.get(cv::CAP_PROP_FPS);
    input_vid_info->codec = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    // the maximum number of frames without motion before video capture is stopped
    const size_t max_no_motion = static_cast<size_t>(std::max<float>(s_pre, s_post)*input_vid_info->fps);

    // A buffer to hold frames before current frame
    struct FrameBuffer prefix_buffer(s_pre*input_vid_info->fps);
    cv::VideoWriter outputVid;
    cv::Mat curr_frame_color;

    cv::Mat curr_frame(input_vid_info->dimensions, CV_MAKETYPE(input_vid_info->format, 1));
    cv::Mat prev_frame = curr_frame.clone();
    cv::Mat prev_prev_frame = prev_frame.clone();

    if(view)
    {
        cv::namedWindow("Current frame");
        cv::createTrackbar("sensitivity", "Current frame", &sensitivity_slider, 100);
        cv::createTrackbar("pixel difference threshold", "Current frame", &motion_threshold, 50);
        cv::createTrackbar("erosion kernel size", "Current frame", &k_size, 10);
    }

    cv::Mat diff1, diff2, motion;
    std::vector<std::vector<cv::Point>> contours;
    uint frame_count = 0;
    while(1)
    {
        cap >> curr_frame_color;
        // make sure the frame isn't empty
        if(curr_frame_color.empty())
        {
            std::cout << "Video stream ended.\n";
            break;
        }

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
        if(view)
            is_motion = static_cast<float>(cv::countNonZero(motion))/input_vid_info->dimensions.area() > sensitivity_slider/10000.0;
        else
            is_motion = static_cast<float>(cv::countNonZero(motion))/input_vid_info->dimensions.area() > motion_sensitivity;

        if(is_motion)
        {
            frames_no_motion = 0;
            if(view)
            cv::putText(curr_frame_color, "MOTION", cv::Point(10,50), cv::FONT_HERSHEY_PLAIN, 3, cv::viz::Color::green());
            
            // create a bounding box around each contour
            cv::findContours(motion, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            // todo: change from boxs around contours to one box around all contours
            //for(auto contour : contours)
            //    cv::rectangle(curr_frame_color, cv::boundingRect(contour), cv::viz::Color::magenta(), 3);
        }
        else
        {
            frames_no_motion++;
        }
        std::string name;
        // if there was motion in the last <max_no_motion> frames save the current frame
        if(frames_no_motion > max_no_motion)
        {
            new_video = true;
            outputVid.release();
            std::cout << "Closed " + name + '\n';
        } else
        {
            if(new_video)
            {
                name = output_path + "OUT_Frame=" + std::to_string(frame_count) + ".mp4";
                outputVid.open(name, input_vid_info->codec, input_vid_info->fps, input_vid_info->dimensions);
                std::cout << "Created " << name << '\n';
                // save previous frames
                prefix_buffer.write_frames(outputVid);
                new_video = false;
            }
            outputVid << curr_frame_color;
        }
        if(view)
        {
            cv::imshow("Current frame", curr_frame_color);
            cv::imshow("AND", motion);
            if(input_vid)
                cv::waitKey(1000/input_vid_info->fps);
        }
        prev_prev_frame = prev_frame.clone();
        prev_frame = curr_frame.clone();
        frame_count++;
    }

    cap.release();
    outputVid.release();
    if(view)
        cv::destroyAllWindows();

    return 0;
}