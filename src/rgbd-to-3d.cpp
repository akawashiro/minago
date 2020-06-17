// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

#include <algorithm> // std::min, std::max
#include <iostream>

/* Attempt at supporting openCV version 4.0.1 or higher */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#define CV_BGR2YCrCb cv::COLOR_BGR2YCrCb
#define CV_HAAR_SCALE_IMAGE cv::CASCADE_SCALE_IMAGE
#define CV_HAAR_FIND_BIGGEST_OBJECT cv::CASCADE_FIND_BIGGEST_OBJECT
#endif

GLuint create_texture(GLuint id, uint8_t *color_data, int width, int height) {

    // テクスチャを拘束
    // NOTICE 以下テクスチャに対する命令は拘束したテクスチャに対して実行される
    glBindTexture(GL_TEXTURE_2D, id);

    // We assume RS2_FORMAT_RGB8
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, color_data);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    // glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    // glBindTexture(GL_TEXTURE_2D, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    return id;
}

int main(int argc, char *argv[]) {
    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    // 初期化
    if (!glfwInit())
        return -1;

    // Window生成
    GLFWwindow *window =
        glfwCreateWindow(1200, 900, "OBJ loader", nullptr, nullptr);

    if (!window) {
        // 生成失敗
        glfwTerminate();
        return -1;
    }

    // OpenGLの命令を使えるようにする
    glfwMakeContextCurrent(window);
    // アプリ画面更新タイミングをPCディスプレイに同期する
    glfwSwapInterval(1);

    // 拡散光と鏡面反射を個別に計算する
    // TIPS:これで、テクスチャを張ったポリゴンもキラーン!!ってなる
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

    // 透視変換行列を設定
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(35.0, 640 / 480.0, 0.2, 200.0);

    // 操作対象の行列をモデリングビュー行列に切り替えておく
    glMatrixMode(GL_MODELVIEW);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    // 並行光源の設定
    GLfloat diffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    GLfloat ambient[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);

    GLfloat specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    std::chrono::system_clock::time_point start, end;

    GLuint id;
    glGenTextures(1, &id);

    while (glfwGetKey(window, GLFW_KEY_Q) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0) {
        start = std::chrono::system_clock::now();

        auto frames = pipe.wait_for_frames();

        auto color = frames.get_color_frame();
        auto depth = frames.get_depth_frame();

        std::cout << "color: " << std::endl
                  << "width = " << color.get_width()
                  << " height = " << color.get_height()
                  << " stride = " << color.get_stride_in_bytes()
                  << " bit per pixel = " << color.get_bits_per_pixel()
                  << std::endl;
        std::cout << "depth: " << std::endl
                  << "width = " << depth.get_width()
                  << " height = " << depth.get_height()
                  << " stride = " << depth.get_stride_in_bytes()
                  << " bit per pixel = " << depth.get_bits_per_pixel()
                  << std::endl;

        // 単位行列を読み込む
        glLoadIdentity();
        glTranslatef(0, 0.0, 0.0);
        gluLookAt(0, 0, 4, 0, 0, 0, 0, 1, 0);

        // 描画バッファの初期化
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ライトの設定
        // GLfloat position[] = {0.0f, 0.0f, 4.0f, 0.0f};
        // glLightfv(GL_LIGHT0, GL_POSITION, position);

        // 描画
        int width = color.get_width();
        int height = color.get_height();
        uint8_t *color_data = (uint8_t *)color.get_data();
        uint16_t *depth_data = (uint16_t *)depth.get_data();

        glPointSize(1.0 / width);
        glBindTexture(GL_TEXTURE_2D, create_texture(id, color_data, width, height));
        glBegin(GL_POINTS);
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                // glColor3f(
                //     ((GLfloat)color_data[i * width * 3 + j * 3 + 0]) / 255,
                //     ((GLfloat)color_data[i * width * 3 + j * 3 + 1]) / 255,
                //     ((GLfloat)color_data[i * width * 3 + j * 3 + 2]) / 255);
                GLfloat x = (GLfloat)(j - width / 2) / height;
                GLfloat y = -(GLfloat)(i - height / 2) / height;
                GLfloat z = -(GLfloat)depth_data[i * width + j] / 5000;
                glVertex3f(x, y, z);
                glTexCoord2f((float)j / width, (float)i / height);

                // std::cout
                //     << "x = " << x << ", y = " << y << ", z = " << z << ", r
                //     = "
                //     << ((GLfloat)color_data[i * width * 3 + j * 3 + 0]) / 255
                //     << ", g = "
                //     << ((GLfloat)color_data[i * width * 3 + j * 3 + 1]) / 255
                //     << ", b = "
                //     << ((GLfloat)color_data[i * width * 3 + j * 3 + 2]) / 255
                //     << std::endl;
            }
        }
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();

        end = std::chrono::system_clock::now();
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}