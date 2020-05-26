#pragma once

#include <utility>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

namespace eye_like {

inline double left_eye_center_x = 0;
inline double left_eye_center_y = 0;
inline double right_eye_center_x = 0;
inline double right_eye_center_y = 0;

struct EyesPosition {
    double left_eye_center_x;
    double left_eye_center_y;
    double right_eye_center_x;
    double right_eye_center_y;
};

void detectAndDisplay(cv::Mat frame);
EyesPosition detect_eyes_position(cv::Mat frame);
int init();
int run_main(std::pair<int, int> resolution, bool enable_image);

} // namespace eye_like