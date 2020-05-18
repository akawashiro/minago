// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include "example.hpp" // Include short list of convenience functions for rendering
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <algorithm> // std::min, std::max

#include "eyeLike.h"

/* Attempt at supporting openCV version 4.0.1 or higher */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#define CV_BGR2YCrCb cv::COLOR_BGR2YCrCb
#define CV_HAAR_SCALE_IMAGE cv::CASCADE_SCALE_IMAGE
#define CV_HAAR_FIND_BIGGEST_OBJECT cv::CASCADE_FIND_BIGGEST_OBJECT
#endif

// Helper functions
void register_glfw_callbacks(window &app, glfw_state &app_state);

int main(int argc, char *argv[]) try {
    std::string main_window_name = "Capture - Face detection";
    std::string face_window_name = "Capture - Face";

    // cv::namedWindow(main_window_name, CV_WINDOW_NORMAL);
    // cv::moveWindow(main_window_name, 400, 100);
    // cv::namedWindow(face_window_name, CV_WINDOW_NORMAL);
    // cv::moveWindow(face_window_name, 10, 100);
    // cv::namedWindow("Right Eye", CV_WINDOW_NORMAL);
    // cv::moveWindow("Right Eye", 10, 600);
    // cv::namedWindow("Left Eye", CV_WINDOW_NORMAL);
    // cv::moveWindow("Left Eye", 10, 800);

    // Create a simple OpenGL window for rendering:
    // window app(1280, 720, "RealSense Pointcloud Example");
    // Construct an object to manage view state
    glfw_state app_state;
    // register callbacks to allow manipulation of the pointcloud
    // register_glfw_callbacks(app, app_state);

    // Declare pointcloud object, for calculating pointclouds and texture
    // mappings
    rs2::pointcloud pc;
    // We want the points object to be persistent so we can display the last
    // cloud when a frame drops
    rs2::points points;

    rs2::config cfg;
    // cfg.enable_stream(RS2_STREAM_COLOR, 720, 500, RS2_FORMAT_BGR8, 30);
    // cfg.enable_stream(RS2_STREAM_DEPTH, 720, 500, RS2_FORMAT_Z16, 30);
    cfg.enable_stream(RS2_STREAM_DEPTH, 1280, 720, RS2_FORMAT_Z16, 30);
    cfg.enable_stream(RS2_STREAM_COLOR, 1280, 720, RS2_FORMAT_RGB8, 30);

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start(cfg);

    std::chrono::system_clock::time_point start, end;

    // while (app) // Application still alive?
    while (cv::waitKey(1) == -1) {
        start = std::chrono::system_clock::now();
        // Wait for the next set of frames from the camera
        auto frames = pipe.wait_for_frames();

        auto color = frames.get_color_frame();
        cv::Mat opencv_color(cv::Size(1280, 720), CV_8UC3,
                             (void *)color.get_data(), cv::Mat::AUTO_STEP);
        cv::Mat screen;
        cv::cvtColor(opencv_color, screen, cv::COLOR_RGB2BGR);
        printf("w = %d, h = %d\n", color.get_width(), color.get_height());
        // if(!opencv_color.empty())
        cv::namedWindow("color", cv::WINDOW_AUTOSIZE);
        cv::imshow("color", screen);

        // // For cameras that don't have RGB sensor, we'll map the pointcloud
        // to
        // // infrared instead of color
        // if (!color)
        //     color = frames.get_infrared_frame();

        // // Tell pointcloud object to map to this color frame
        // pc.map_to(color);

        // auto depth = frames.get_depth_frame();

        // // Generate the pointcloud and texture mappings
        // points = pc.calculate(depth);

        // // Upload the color frame to OpenGL
        // app_state.tex.upload(color);

        // // Draw the pointcloud
        // // draw_pointcloud(app.width(), app.height(), app_state, points);

        // end = std::chrono::system_clock::now();
        // double time = static_cast<double>(
        //     std::chrono::duration_cast<std::chrono::microseconds>(end -
        //     start)
        //         .count() /
        //     1000.0);
        // printf("time %lf[ms]\n", time);
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
