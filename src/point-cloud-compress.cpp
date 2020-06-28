#include "camera.h"

#include <librealsense2/rs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <iostream>

/* Attempt at supporting openCV version 4.0.1 or higher */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#define CV_BGR2YCrCb cv::COLOR_BGR2YCrCb
#define CV_HAAR_SCALE_IMAGE cv::CASCADE_SCALE_IMAGE
#define CV_HAAR_FIND_BIGGEST_OBJECT cv::CASCADE_FIND_BIGGEST_OBJECT
#endif

int main() {
    // Declare RealSense pipeline, encapsulating the actual device and
    // sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    auto profile = pipe.start();

    for (auto &&sensor : profile.get_device().query_sensors()) {
        sensor.set_option(RS2_OPTION_FRAMES_QUEUE_SIZE, 0);
    }

    rs2::pointcloud pc;
    rs2::points points;

    bool is_first = true;
    cv::Mat red_old;
    cv::Mat green_old;
    cv::Mat blue_old;
    while (1) {
        // Wait for the next set of frames from the camera
        auto frames = pipe.wait_for_frames();

        auto depth = frames.get_depth_frame();
        auto color = frames.get_color_frame();

        cv::Mat opencv_color(cv::Size(1280, 720), CV_8UC3,
                             (void *)color.get_data(), cv::Mat::AUTO_STEP);
        cv::Mat screen;
        cv::cvtColor(opencv_color, screen, cv::COLOR_RGB2BGR);

        depth.keep();
        color.keep();
        {
            camera::rs2_frame_data f;
            f.height = color.get_height();
            f.width = color.get_width();
            std::shared_ptr<uint8_t> rgb_tmp(
                new uint8_t[3 * f.width * f.height],
                std::default_delete<uint8_t[]>());
            f.rgb = rgb_tmp;
            memcpy(f.rgb.get(), color.get_data(),
                   sizeof(uint8_t) * 3 * f.width * f.height);

            pc.map_to(color);
            points = pc.calculate(depth);
            f.n_points = points.size();
            std::shared_ptr<rs2::vertex> vertices_tmp(
                new rs2::vertex[f.n_points],
                std::default_delete<rs2::vertex[]>());
            f.vertices = vertices_tmp;
            memcpy(f.vertices.get(), points.get_vertices(),
                   sizeof(rs2::vertex) * f.n_points);
            std::shared_ptr<rs2::texture_coordinate> texture_coordinates_tmp(
                new rs2::texture_coordinate[f.n_points],
                std::default_delete<rs2::texture_coordinate[]>());
            f.texture_coordinates = texture_coordinates_tmp;
            memcpy(f.texture_coordinates.get(),
                   points.get_texture_coordinates(),
                   sizeof(rs2::texture_coordinate) * f.n_points);

            std::cout << "========== one frame start ==========" << std::endl;
            float min_x = std::numeric_limits<float>::max();
            float min_y = std::numeric_limits<float>::max();
            float min_z = std::numeric_limits<float>::max();
            float max_x = -std::numeric_limits<float>::max();
            float max_y = -std::numeric_limits<float>::max();
            float max_z = -std::numeric_limits<float>::max();

            for (int i = 0; i < f.n_points; i++) {
                rs2::vertex v = f.vertices[i];
                min_x = std::min(min_x, v.x);
                min_y = std::min(min_y, v.y);
                min_z = std::min(min_z, v.z);
                max_x = std::max(max_x, v.x);
                max_y = std::max(max_y, v.y);
                max_z = std::max(max_z, v.z);
            }
            max_x -= min_x;
            max_y -= min_y;
            max_z -= min_z;
            for (int i = 0; i < f.n_points; i++) {
                f.vertices[i].x = (f.vertices[i].x - min_x) / max_x;
                f.vertices[i].y = (f.vertices[i].y - min_y) / max_y;
                f.vertices[i].z = (f.vertices[i].z - min_z) / max_z;
            }

            float min_u = std::numeric_limits<float>::max();
            float min_v = std::numeric_limits<float>::max();
            float max_u = -std::numeric_limits<float>::max();
            float max_v = -std::numeric_limits<float>::max();

            for (int i = 0; i < f.n_points; i++) {
                rs2::texture_coordinate t = f.texture_coordinates[i];
                min_u = std::min(min_x, t.u);
                min_v = std::min(min_y, t.v);
                max_u = std::max(max_x, t.u);
                max_v = std::max(max_y, t.v);
            }
            max_u -= min_u;
            max_v -= min_v;
            for (int i = 0; i < f.n_points; i++) {
                f.texture_coordinates[i].u =
                    (f.texture_coordinates[i].u - min_u) / max_u;
                f.texture_coordinates[i].v =
                    (f.texture_coordinates[i].v - min_v) / max_v;
            }

            cv::Mat pc_image(f.height, f.width, CV_32FC3, f.vertices.get());
            cv::Mat pc_image_16uc3;
            pc_image.convertTo(pc_image_16uc3, CV_16UC3, 65535.0);

            cv::Mat red(pc_image_16uc3.rows, pc_image_16uc3.cols, CV_16UC1);
            cv::Mat green(pc_image_16uc3.rows, pc_image_16uc3.cols, CV_16UC1);
            cv::Mat blue(pc_image_16uc3.rows, pc_image_16uc3.cols, CV_16UC1);

            cv::Mat red_res;
            cv::Mat green_res;
            cv::Mat blue_res;

            // forming an array of matrices is a quite efficient operation,
            // because the matrix data is not copied, only the headers
            cv::Mat out[] = {red, green, blue};
            int from_to[] = {0, 0, 1, 1, 2, 2};
            mixChannels(&pc_image_16uc3, 1, out, 3, from_to, 3);

            cv::Mat tx_image(f.height, f.width, CV_32FC2,
                             f.texture_coordinates.get());
            cv::Mat tx_image_16uc3;
            tx_image.convertTo(tx_image_16uc3, CV_16UC3, 65535.0);

            cv::Mat tx_u(tx_image_16uc3.rows, tx_image_16uc3.cols, CV_16UC1);
            cv::Mat tx_v(tx_image_16uc3.rows, tx_image_16uc3.cols, CV_16UC1);

            // forming an array of matrices is a quite efficient operation,
            // because the matrix data is not copied, only the headers
            cv::Mat tx_out[] = {tx_u, tx_v};
            int tx_from_to[] = {0, 0, 1, 1};
            mixChannels(&tx_image_16uc3, 1, tx_out, 2, tx_from_to, 2);

            if (!is_first) {
                // cv::subtract(red, red_old, red_res, cv::Mat(), -1);
                red_res = red - red_old;
                green_res = green - green_old;
                blue_res = blue - blue_old;
                // cv::subtract(green, green_old, green_res, cv::Mat(), -1);
                // cv::subtract(blue, blue_old, blue_res, cv::Mat(), -1);
            } else {
                red_res = red;
                green_res = green;
                blue_res = blue;
            }

            red_old = red;
            green_old = green;
            blue_old = blue;
            is_first = false;

            std::vector<int> compression_params;
            compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
            compression_params.push_back(9);

            cv::imwrite("pc_image_x.bmp", red);
            cv::imwrite("pc_image_y.bmp", green);
            cv::imwrite("pc_image_z.bmp", blue);
            cv::imwrite("pc_image_x.jpg", red);
            cv::imwrite("pc_image_y.jpg", green);
            cv::imwrite("pc_image_z.jpg", blue);
            cv::imwrite("pc_image_x.png", red, compression_params);
            cv::imwrite("pc_image_y.png", green, compression_params);
            cv::imwrite("pc_image_z.png", blue, compression_params);

            cv::imwrite("tx_image_u.bmp", tx_u);
            cv::imwrite("tx_image_v.bmp", tx_v);

            cv::imwrite("pc_image_x_res.bmp", red_res);
            cv::imwrite("pc_image_y_res.bmp", green_res);
            cv::imwrite("pc_image_z_res.bmp", blue_res);
            cv::imwrite("pc_image_x_res.jpg", red_res);
            cv::imwrite("pc_image_y_res.jpg", green_res);
            cv::imwrite("pc_image_z_res.jpg", blue_res);
            cv::imwrite("pc_image_x_res.png", red_res, compression_params);
            cv::imwrite("pc_image_y_res.png", green_res, compression_params);
            cv::imwrite("pc_image_z_res.png", blue_res, compression_params);

            cv::Mat load_img = cv::imread("pc_image_x.png", CV_16UC1);
            // cv::Mat load_img_32fc3;
            // load_img.convertTo(load_img_32fc3, CV_16UC3);
            for (int i = 0; i < f.n_points; i++) {
                if (i % 1000 == 0) {
                    uint16_t *p_16uc3 = (uint16_t *)pc_image_16uc3.data;
                    uint16_t *p_red = (uint16_t *)red.data;
                    uint16_t *p_load = (uint16_t *)load_img.data;
                    std::cout << "f.vertices[i].x = " << f.vertices[i].x
                              << ", p_16uc3[i * 3 + 0] = "
                              << p_16uc3[i * 3 + 0] / 65535.0
                              << ", p_red[i] = " << p_red[i] / 65535.0
                              << ", p_load[i] = " << p_load[i] / 65535.0
                              << std::endl;
                }
            }
        }
    }

    return 0;
}