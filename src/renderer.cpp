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
namespace {
void upload_texture(uint8_t *color_data, int width, int height, GLuint &id) {
    if (!id)
        glGenTextures(1, &id);
    GLenum err = glGetError();

    glBindTexture(GL_TEXTURE_2D, id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_BGR,
                 GL_UNSIGNED_BYTE, color_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Handles all the OpenGL calls needed to display the point cloud
void draw_pointcloud_render(float width, float height, glfw_state &app_state,
                            eye_like::EyesPosition eye_position, int n_point,
                            const rs2::vertex *vertices,
                            const rs2::texture_coordinate *tex_coords,
                            GLuint gl_texture_id) {
    double eyex =
        (eye_position.left_eye_center_x + eye_position.right_eye_center_x) / 2 -
        0.5;
    double eyey =
        -(eye_position.left_eye_center_y + eye_position.right_eye_center_y) /
            2 +
        0.5;
    const double scale_x = 1.0;
    const double scale_y = scale_x / 9 * 16;
    eyex *= scale_x;
    eyey *= scale_y;
    // std::cout << "eyex = " << eyex << ", eyey = " << eyey << std::endl;

    // OpenGL commands that prep screen for the pointcloud
    glLoadIdentity();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glClearColor(153.f / 255, 153.f / 255, 153.f / 255, 1);
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    gluPerspective(60, width / height, 0.01f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    // gluLookAt(0, -0.2, 0, 0, 0, 1, 0, -1, 0);
    gluLookAt(-eyex, -eyey, 0, 0, 0, 1, 0, -1, 0);

    glTranslatef(0, 0, +0.5f + app_state.offset_y * 0.05f);
    glRotated(app_state.pitch, 1, 0, 0);
    glRotated(app_state.yaw, 0, 1, 0);
    glTranslatef(0, 0, -0.5f);

    glPointSize(width / 640);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, gl_texture_id);
    float tex_border_color[] = {0.8f, 0.8f, 0.8f, 0.8f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    0x812F); // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    0x812F); // GL_CLAMP_TO_EDGE
    glBegin(GL_POINTS);

    // float max_x = FLT_MIN, min_x = FLT_MAX;
    // float max_y = FLT_MIN, min_y = FLT_MAX;
    // float max_z = FLT_MIN, min_z = FLT_MAX;

    for (int i = 0; i < n_point; i++) {
        if (vertices[i].z) {
            // upload the point and texture coordinates only for points we have
            // depth data for
            glVertex3fv(vertices[i]);
            glTexCoord2fv(tex_coords[i]);

            // max_x = std::max(max_x, vertices[i].x);
            // min_x = std::min(min_x, vertices[i].x);
            // max_y = std::max(max_y, vertices[i].y);
            // min_y = std::min(min_y, vertices[i].y);
            // max_z = std::max(max_z, vertices[i].z);
            // min_z = std::min(min_z, vertices[i].z);
        }
    }
    // std::cout << "max_x = " << max_x << ", min_x = " << min_x << std::endl;
    // std::cout << "max_y = " << max_y << ", min_y = " << min_y << std::endl;
    // std::cout << "max_z = " << max_z << ", min_z = " << min_z << std::endl;

    // OpenGL cleanup
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}
} // namespace

int renderer_main_loop(
    ThreadSafeState<eye_like::EyesPosition>::ThreadSafeStateGetViewer
        &eye_pos_get,
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePopViewer
        &frame_queue) {
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "minago - 3d telecommunication software");
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
    eye_like::EyesPosition eye_position =
        eye_like::EyesPosition{0.0, 0.0, 0.0, 0.0};

    GLuint gl_texture_id = 0;

    int n_points;
    std::shared_ptr<rs2::vertex[]> vertices;
    std::shared_ptr<rs2::texture_coordinate[]> texture_coordinates;

    while (app) { // Application still alive?
        start = std::chrono::system_clock::now();
        if (!frame_queue.empty()) {
            auto f = frame_queue.pop();

            int height = f->height;
            int width = f->width;
            n_points = f->n_points;
            vertices = f->vertices;
            texture_coordinates = f->texture_coordinates;

            upload_texture(f->rgb.get(), width, height, gl_texture_id);
        }
        if (vertices && texture_coordinates) {
            eye_position = eye_pos_get.get();
            draw_pointcloud_render(app.width(), app.height(), app_state,
                                   eye_position, n_points, vertices.get(),
                                   texture_coordinates.get(), gl_texture_id);
        }

        end = std::chrono::system_clock::now();
        double time = static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(end - start)
                .count() /
            1000.0);
        // printf("time %lf[ms]\n", time);
    }

    return EXIT_SUCCESS;
}
} // namespace renderer