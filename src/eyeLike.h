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

void detectAndDisplay(cv::Mat frame);
int init();
int run_main(std::pair<int, int> resolution, bool enable_image);

} // namespace eye_like