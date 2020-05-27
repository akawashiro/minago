#include "camera.h"

#include <librealsense2/rs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <algorithm>
#include <iostream>

/* Attempt at supporting openCV version 4.0.1 or higher */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#define CV_BGR2YCrCb cv::COLOR_BGR2YCrCb
#define CV_HAAR_SCALE_IMAGE cv::CASCADE_SCALE_IMAGE
#define CV_HAAR_FIND_BIGGEST_OBJECT cv::CASCADE_FIND_BIGGEST_OBJECT
#endif

namespace camera {
int camera_main_loop(
    ThreadSafeQueuePushViewer<eye_like::EyesPosition> &eye_pos_queue,
    ThreadSafeQueuePushViewer<rs2_frame_data> &frame_queue) try {
    eye_like::init();

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    auto profile = pipe.start();

    for (auto &&sensor : profile.get_device().query_sensors()) {
        sensor.set_option(RS2_OPTION_FRAMES_QUEUE_SIZE, 0);
    }

    std::chrono::system_clock::time_point start, end;

    while (1) {
        start = std::chrono::system_clock::now();
        // Wait for the next set of frames from the camera
        auto frames = pipe.wait_for_frames();

        auto depth = frames.get_depth_frame();
        auto color = frames.get_color_frame();

        cv::Mat opencv_color(cv::Size(1280, 720), CV_8UC3,
                             (void *)color.get_data(), cv::Mat::AUTO_STEP);
        cv::Mat screen;
        cv::cvtColor(opencv_color, screen, cv::COLOR_RGB2BGR);
        eye_like::EyesPosition eyes_position;
        if (!screen.empty()) {
            cv::namedWindow("color", cv::WINDOW_AUTOSIZE);
            eye_like::detectAndDisplay(screen);
            eyes_position = eye_like::detect_eyes_position(screen);
            eye_pos_queue.push(eyes_position);
        }

        depth.keep();
        color.keep();

        frame_queue.push({depth, color});

        double eyex = (eyes_position.left_eye_center_x +
                       eyes_position.right_eye_center_x) /
                          2 -
                      0.5;
        double eyey = -(eyes_position.left_eye_center_y +
                        eyes_position.right_eye_center_y) /
                          2 +
                      0.5;
        const double scale_x = 1.0;
        const double scale_y = scale_x / 9 * 16;
        eyex *= scale_x;
        eyey *= scale_y;
        std::cout << "eyex = " << eyex << ", eyey = " << eyey << std::endl;

        end = std::chrono::system_clock::now();
        double time = static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                .count() /
            1000.0);
        std::cout << "time " << time << "[ms]" << std::endl;
    }

    return EXIT_SUCCESS;
} catch (const rs2::error &e) {
    std::cerr << "RealSense error calling " << e.get_failed_function() << "("
              << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
} // namespace camera