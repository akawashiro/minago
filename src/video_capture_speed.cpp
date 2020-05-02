#include <opencv2/highgui/highgui.hpp>

#include <chrono>
#include <iostream>

int main() {
    std::chrono::system_clock::time_point start, end;
    cv::Mat frame;
    cv::VideoCapture capture(0);
    if (capture.isOpened()) {
        start = std::chrono::system_clock::now();
        while (true) {
            capture.read(frame);
            end = std::chrono::system_clock::now();
            double time = static_cast<double>(
                std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                      start)
                    .count() /
                1000.0);
            printf("time %lf[ms]\n", time);
            start = end;
        }
    }
    return true;
}