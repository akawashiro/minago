#include "camera.h"

#include <librealsense2/rs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <signal.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

/* Attempt at supporting openCV version 4.0.1 or higher */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#define CV_BGR2YCrCb cv::COLOR_BGR2YCrCb
#define CV_HAAR_SCALE_IMAGE cv::CASCADE_SCALE_IMAGE
#define CV_HAAR_FIND_BIGGEST_OBJECT cv::CASCADE_FIND_BIGGEST_OBJECT
#endif

namespace camera {
int camera_main_loop(
    ThreadSafeState<eye_like::EyesPosition>::ThreadSafeStatePutViewer
        &eye_pos_put,
    ThreadSafeQueue<rs2_frame_data>::ThreadSafeQueuePushViewer
        &frame_queue) try {
    eye_like::init();

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;

    std::atomic<bool> realsense_available = false;
    rs2::pipeline_profile profile;

    bool webcam_available = false;
    cv::VideoCapture cap;

    // Start streaming with default recommended configuration
    std::thread realsense_init([&]() {
        profile = pipe.start();
        realsense_available = true;
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (realsense_available) {
        realsense_init.join();
        std::cout << "RealSense is available." << std::endl;
    } else {
        try {
            pthread_cancel(realsense_init.native_handle());
        } catch (std::exception &e) {
            std::cout << e.what() << std::endl;
        }
        std::cout
            << "RealSense is not available. We try to use a normal webcam."
            << std::endl;

        cap.open(0);
        if (cap.isOpened()) {
            webcam_available = true;
            std::cout << "Webcam is available." << std::endl;
        } else {
            std::cout << "Both of Realsense and webcam are not available. We "
                         "terminate this program."
                      << std::endl;
            return 0;
        }
    }

    if (realsense_available) {
        for (auto &&sensor : profile.get_device().query_sensors()) {
            sensor.set_option(RS2_OPTION_FRAMES_QUEUE_SIZE, 0);
        }

        rs2::pointcloud pc;
        rs2::points points;

        while (1) {
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
                eye_like::detectAndDisplay(screen);
                eyes_position = eye_like::detect_eyes_position(screen);
                eye_pos_put.put(eyes_position);
            }

            depth.keep();
            color.keep();
            {
                rs2_frame_data f;
                f.height = color.get_height();
                f.width = color.get_width();
                std::shared_ptr<uint8_t[]> rgb_tmp(
                    new uint8_t[3 * f.width * f.height]);
                f.rgb = rgb_tmp;
                memcpy(f.rgb.get(), color.get_data(),
                       sizeof(uint8_t) * 3 * f.width * f.height);

                pc.map_to(color);
                points = pc.calculate(depth);
                f.n_points = points.size();
                std::shared_ptr<rs2::vertex[]> vertices_tmp(
                    new rs2::vertex[f.n_points]);
                f.vertices = vertices_tmp;
                memcpy(f.vertices.get(), points.get_vertices(),
                       sizeof(rs2::vertex) * f.n_points);
                std::shared_ptr<rs2::texture_coordinate[]>
                    texture_coordinates_tmp(
                        new rs2::texture_coordinate[f.n_points]);
                f.texture_coordinates = texture_coordinates_tmp;
                memcpy(f.texture_coordinates.get(),
                       points.get_texture_coordinates(),
                       sizeof(rs2::texture_coordinate) * f.n_points);

                frame_queue.push(f);
            }
        }
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