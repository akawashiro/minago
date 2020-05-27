#include "renderer.h"

#include "example.hpp" // Include short list of convenience functions for rendering
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect/objdetect.hpp>

#include <algorithm> // std::min, std::max

/* Attempt at supporting openCV version 4.0.1 or higher */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#define CV_BGR2YCrCb cv::COLOR_BGR2YCrCb
#define CV_HAAR_SCALE_IMAGE cv::CASCADE_SCALE_IMAGE
#define CV_HAAR_FIND_BIGGEST_OBJECT cv::CASCADE_FIND_BIGGEST_OBJECT
#endif

// Helper functions
void register_glfw_callbacks(window &app, glfw_state &app_state);

namespace renderer {
int renderer_main_loop(
    ThreadSafeQueuePopViewer<eye_like::EyesPosition> &pos_queue,
    ThreadSafeQueuePopViewer<camera::rs2_frame_data> &frame_queue) {
    std::string main_window_name = "Capture - Face detection";
    std::string face_window_name = "Capture - Face";

    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Pointcloud Example");
    // Construct an object to manage view state
    glfw_state app_state;
    // register callbacks to allow manipulation of the pointcloud
    register_glfw_callbacks(app, app_state);

    // Declare pointcloud object, for calculating pointclouds and texture
    // mappings
    rs2::pointcloud pc;
    // We want the points object to be persistent so we can display the last
    // cloud when a frame drops
    rs2::points points;

    std::chrono::system_clock::time_point start, end;

    while (app) { // Application still alive?
        start = std::chrono::system_clock::now();
        if (!frame_queue.empty()) {
            auto f = frame_queue.pop();

            // Tell pointcloud object to map to this color frame
            pc.map_to(f->color);

            // Generate the pointcloud and texture mappings
            points = pc.calculate(f->depth);

            // Upload the color frame to OpenGL
            app_state.tex.upload(f->color);
        }

        // Draw the pointcloud
        draw_pointcloud(app.width(), app.height(), app_state, points);

        end = std::chrono::system_clock::now();
        double time = static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                .count() /
            1000.0);
        printf("time %lf[ms]\n", time);
    }

    return EXIT_SUCCESS;
}
} // namespace renderer