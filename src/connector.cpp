#include "connector.h"

#include "compress.h"

#include <opencv2/core/core.hpp>

#include <arpa/inet.h>
#include <cassert>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>

namespace connector {

const double ABS_MAX_16SU = (1 << 12) - 1;

void print_mat_u8(const cv::Mat &mat) {
    for (int i = 0; i < mat.rows; i++) {
        for (int j = 0; j < mat.cols; j++) {
            std::cout << (int)(((uint8_t *)mat.data)[i * mat.cols + j]) << " ";
        }
        std::cout << std::endl;
    }
}

void print_mat_u16(const cv::Mat &mat) {
    for (int i = 0; i < mat.rows; i++) {
        for (int j = 0; j < mat.cols; j++) {
            std::cout << ((short *)mat.data)[i * mat.cols + j] << " ";
        }
        std::cout << std::endl;
    }
}

uint32_t serialize_frame_data(camera::rs2_frame_data frame, char *buf) {
    char *p = buf;

    // I write the length of this frame at the last.
    p += sizeof(uint32_t);

    *((uint32_t *)p) = frame.height;
    p += sizeof(uint32_t);

    *((uint32_t *)p) = frame.width;
    p += sizeof(uint32_t);

    *((uint32_t *)p) = frame.n_points;
    p += sizeof(uint32_t);

    char *compress_output =
        (char *)malloc(frame.n_points * sizeof(rs2::vertex));
    int compress_length;

    // RGB
    {
        cv::Mat rgb_image(frame.height, frame.width, CV_8UC3, frame.rgb.get());

        std::vector<uchar> jpeg_buf;
        cv::imencode(".jpg", rgb_image, jpeg_buf);
        BOOST_LOG_TRIVIAL(info) << "The size of jpeg_buf = " << jpeg_buf.size();
        *((uint32_t *)p) = jpeg_buf.size();
        p += sizeof(uint32_t);
        memcpy(p, jpeg_buf.data(), jpeg_buf.size());
        p += jpeg_buf.size();
    }

    // XYZ
    {
        cv::Mat xyz_image(frame.height, frame.width, CV_32FC3,
                          frame.vertices.get());
        cv::Mat x32f(frame.height, frame.width, CV_32FC1);
        cv::Mat y32f(frame.height, frame.width, CV_32FC1);
        cv::Mat z32f(frame.height, frame.width, CV_32FC1);

        cv::Mat out_xyz[] = {x32f, y32f, z32f};
        int from_to_xyz[] = {0, 0, 1, 1, 2, 2};
        mixChannels(&xyz_image, 1, out_xyz, 3, from_to_xyz, 3);

        // Normalization
        double max_x, min_x, max_y, min_y, max_z, min_z;
        cv::minMaxLoc(x32f, &min_x, &max_x);
        cv::minMaxLoc(y32f, &min_y, &max_y);
        cv::minMaxLoc(z32f, &min_z, &max_z);

        x32f = x32f - min_x;
        y32f = y32f - min_y;
        z32f = z32f - min_z;

        std::shared_ptr<cv::Mat> x16u =
            std::make_shared<cv::Mat>(cv::Mat(x32f.rows, x32f.cols, CV_16SC1));
        std::shared_ptr<cv::Mat> y16u =
            std::make_shared<cv::Mat>(cv::Mat(y32f.rows, y32f.cols, CV_16SC1));
        std::shared_ptr<cv::Mat> z16u =
            std::make_shared<cv::Mat>(cv::Mat(z32f.rows, z32f.cols, CV_16SC1));
        x32f.convertTo(*x16u, CV_16SC1, ABS_MAX_16SU / (max_x - min_x));
        y32f.convertTo(*y16u, CV_16SC1, ABS_MAX_16SU / (max_y - min_y));
        z32f.convertTo(*z16u, CV_16SC1, ABS_MAX_16SU / (max_z - min_z));

        compress((char *)(*x16u).data,
                 frame.height * frame.width * sizeof(uint16_t), compress_output,
                 &compress_length);
        BOOST_LOG_TRIVIAL(info) << "x: original size = "
                                << frame.height * frame.width * sizeof(uint16_t)
                                << ", compressed size = " << compress_length;

        *((float *)p) = (float)((max_x - min_x) / ABS_MAX_16SU);
        p += sizeof(float);
        *((float *)p) = (float)min_x;
        p += sizeof(float);
        *((uint32_t *)p) = compress_length;
        p += sizeof(uint32_t);
        memcpy(p, compress_output, compress_length);
        p += compress_length;

        compress((char *)(*y16u).data,
                 frame.height * frame.width * sizeof(uint16_t), compress_output,
                 &compress_length);
        BOOST_LOG_TRIVIAL(info) << "y: original size = "
                                << frame.height * frame.width * sizeof(uint16_t)
                                << ", compressed size = " << compress_length;

        *((float *)p) = (float)((max_y - min_y) / ABS_MAX_16SU);
        p += sizeof(float);
        *((float *)p) = (float)min_y;
        p += sizeof(float);
        *((uint32_t *)p) = compress_length;
        p += sizeof(uint32_t);
        memcpy(p, compress_output, compress_length);
        p += compress_length;

        compress((char *)(*z16u).data,
                 frame.height * frame.width * sizeof(uint16_t), compress_output,
                 &compress_length);
        BOOST_LOG_TRIVIAL(info) << "z: original size = "
                                << frame.height * frame.width * sizeof(uint16_t)
                                << ", compressed size = " << compress_length;
        *((float *)p) = (float)((max_z - min_z) / ABS_MAX_16SU);
        p += sizeof(float);
        *((float *)p) = (float)min_z;
        p += sizeof(float);
        *((uint32_t *)p) = compress_length;
        p += sizeof(uint32_t);
        memcpy(p, compress_output, compress_length);
        p += compress_length;
    }

    // UV
    {
        cv::Mat uv_image(frame.height, frame.width, CV_32FC2,
                         frame.texture_coordinates.get());
        cv::Mat u32f(frame.height, frame.width, CV_32FC1);
        cv::Mat v32f(frame.height, frame.width, CV_32FC1);

        cv::Mat out_uv[] = {u32f, v32f};
        int from_to_uv[] = {0, 0, 1, 1};
        mixChannels(&uv_image, 1, out_uv, 2, from_to_uv, 2);

        // Normalization
        double max_u, min_u, max_v, min_v;
        cv::minMaxLoc(u32f, &min_u, &max_u);
        cv::minMaxLoc(v32f, &min_v, &max_v);

        u32f = u32f - min_u;
        v32f = v32f - min_v;

        std::shared_ptr<cv::Mat> u16u =
            std::make_shared<cv::Mat>(cv::Mat(u32f.rows, u32f.cols, CV_16SC1));
        std::shared_ptr<cv::Mat> v16u =
            std::make_shared<cv::Mat>(cv::Mat(v32f.rows, v32f.cols, CV_16SC1));
        u32f.convertTo(*u16u, CV_16SC1, frame.width / (max_u - min_u));
        v32f.convertTo(*v16u, CV_16SC1, frame.height / (max_v - min_v));

        compress((char *)(*u16u).data,
                 frame.height * frame.width * sizeof(uint16_t), compress_output,
                 &compress_length);
        BOOST_LOG_TRIVIAL(info) << "u: original size = "
                                << frame.height * frame.width * sizeof(uint16_t)
                                << ", compressed size = " << compress_length;
        *((float *)p) = (float)((max_u - min_u) / frame.width);
        p += sizeof(float);
        *((float *)p) = (float)min_u;
        p += sizeof(float);
        *((uint32_t *)p) = compress_length;
        p += sizeof(uint32_t);
        memcpy(p, compress_output, compress_length);
        p += compress_length;

        compress((char *)(*v16u).data,
                 frame.height * frame.width * sizeof(uint16_t), compress_output,
                 &compress_length);
        BOOST_LOG_TRIVIAL(info) << "v: original size = "
                                << frame.height * frame.width * sizeof(uint16_t)
                                << ", compressed size = " << compress_length;
        *((float *)p) = (float)((max_v - min_v) / frame.height);
        p += sizeof(float);
        *((float *)p) = (float)min_v;
        p += sizeof(float);
        *((uint32_t *)p) = compress_length;
        p += sizeof(uint32_t);
        memcpy(p, compress_output, compress_length);
        p += compress_length;
    }

    // Write the size of compressed frame to the head.
    *((uint32_t *)buf) = p - buf;

    free(compress_output);

    return p - buf;
}

camera::rs2_frame_data deserialize_frame_data(char *buf) {
    camera::rs2_frame_data frame;
    char *p = buf;

    char *compress_output =
        (char *)malloc(frame.n_points * sizeof(rs2::vertex));
    int compress_length;

    // Skip the first 4 bytes of the length of the serialized data.
    p += sizeof(uint32_t);

    frame.height = *((uint32_t *)p);
    p += sizeof(uint32_t);

    frame.width = *((uint32_t *)p);
    p += sizeof(uint32_t);

    frame.n_points = *((uint32_t *)p);
    p += sizeof(uint32_t);

    // Extract RGB information
    // These memories are directly used as cv::Mat buffer.
    {
        uint32_t jpeg_size = *((uint32_t *)p);
        p += sizeof(uint32_t);
        std::vector<uchar> jpeg_buf(jpeg_size);
        memcpy(jpeg_buf.data(), p, jpeg_size);
        p += jpeg_size;

        cv::Mat rgb_image = cv::imdecode(jpeg_buf, cv::IMREAD_COLOR);

        std::shared_ptr<uint8_t> rgb_tmp(
            new uint8_t[3 * frame.width * frame.height],
            std::default_delete<uint8_t[]>());
        frame.rgb = rgb_tmp;
        memcpy(frame.rgb.get(), rgb_image.data,
               sizeof(uint8_t) * 3 * frame.width * frame.height);
    }

    // XYZ
    {
        char *x16u_buf =
            (char *)malloc(frame.width * frame.height * sizeof(uint16_t));
        char *y16u_buf =
            (char *)malloc(frame.width * frame.height * sizeof(uint16_t));
        char *z16u_buf =
            (char *)malloc(frame.width * frame.height * sizeof(uint16_t));

        float x_magnification = *((float *)p);
        p += sizeof(float);
        float x_bias = *((float *)p);
        p += sizeof(float);
        uint32_t x_comp_length = *((uint32_t *)p);
        p += sizeof(uint32_t);
        int x_decomp_length = frame.width * frame.height * sizeof(uint16_t);
        decompress(p, x_comp_length, x16u_buf, &x_decomp_length);
        p += x_comp_length;
        BOOST_LOG_TRIVIAL(info) << "x_comp_length = " << x_comp_length
                                << ", x_decomp_length = " << x_decomp_length;

        float y_magnification = *((float *)p);
        p += sizeof(float);
        float y_bias = *((float *)p);
        p += sizeof(float);
        uint32_t y_comp_length = *((uint32_t *)p);
        p += sizeof(uint32_t);
        int y_decomp_length = frame.width * frame.height * sizeof(uint16_t);
        decompress(p, y_comp_length, y16u_buf, &y_decomp_length);
        p += y_comp_length;
        BOOST_LOG_TRIVIAL(info) << "y_comp_length = " << y_comp_length
                                << ", y_decomp_length = " << y_decomp_length;

        float z_magnification = *((float *)p);
        p += sizeof(float);
        float z_bias = *((float *)p);
        p += sizeof(float);
        uint32_t z_comp_length = *((uint32_t *)p);
        p += sizeof(uint32_t);
        int z_decomp_length = frame.width * frame.height * sizeof(uint16_t);
        decompress(p, z_comp_length, z16u_buf, &z_decomp_length);
        p += z_comp_length;
        BOOST_LOG_TRIVIAL(info) << "z_comp_length = " << z_comp_length
                                << ", z_decomp_length = " << z_decomp_length;

        cv::Mat x16u(frame.height, frame.width, CV_16SC1, x16u_buf);
        cv::Mat y16u(frame.height, frame.width, CV_16SC1, y16u_buf);
        cv::Mat z16u(frame.height, frame.width, CV_16SC1, z16u_buf);

        cv::Mat x32f(frame.height, frame.width, CV_32FC1);
        cv::Mat y32f(frame.height, frame.width, CV_32FC1);
        cv::Mat z32f(frame.height, frame.width, CV_32FC1);
        cv::Mat xyz_image(frame.height, frame.width, CV_32FC3);

        x16u.convertTo(x32f, CV_32FC1, x_magnification, x_bias);
        y16u.convertTo(y32f, CV_32FC1, y_magnification, y_bias);
        z16u.convertTo(z32f, CV_32FC1, z_magnification, z_bias);

        cv::Mat in_xyz[] = {x32f, y32f, z32f};
        int from_to_xyz[] = {0, 0, 1, 1, 2, 2};
        mixChannels(in_xyz, 3, &xyz_image, 1, from_to_xyz, 3);

        std::shared_ptr<rs2::vertex> vertices_tmp(
            new rs2::vertex[frame.n_points],
            std::default_delete<rs2::vertex[]>());
        frame.vertices = vertices_tmp;
        memcpy(frame.vertices.get(), xyz_image.data,
               sizeof(rs2::vertex) * frame.n_points);
    }

    // UV
    {
        char *u16u_buf =
            (char *)malloc(frame.width * frame.height * sizeof(uint16_t));
        char *v16u_buf =
            (char *)malloc(frame.width * frame.height * sizeof(uint16_t));

        float u_magnification = *((float *)p);
        p += sizeof(float);
        float u_bias = *((float *)p);
        p += sizeof(float);
        uint32_t u_comp_length = *((uint32_t *)p);
        p += sizeof(uint32_t);
        int u_decomp_length = frame.width * frame.height * sizeof(uint16_t);
        decompress(p, u_comp_length, u16u_buf, &u_decomp_length);
        p += u_comp_length;
        BOOST_LOG_TRIVIAL(info) << "u_comp_length = " << u_comp_length
                                << ", u_decomp_length = " << u_decomp_length;

        float v_magnification = *((float *)p);
        p += sizeof(float);
        float v_bias = *((float *)p);
        p += sizeof(float);
        uint32_t v_comp_length = *((uint32_t *)p);
        p += sizeof(uint32_t);
        int v_decomp_length = frame.width * frame.height * sizeof(uint16_t);
        decompress(p, v_comp_length, v16u_buf, &v_decomp_length);
        p += v_comp_length;
        BOOST_LOG_TRIVIAL(info) << "v_comp_length = " << v_comp_length
                                << ", v_decomp_length = " << v_decomp_length;

        cv::Mat u16u(frame.height, frame.width, CV_16SC1, u16u_buf);
        cv::Mat v16u(frame.height, frame.width, CV_16SC1, v16u_buf);

        cv::Mat u32f(frame.height, frame.width, CV_32FC1);
        cv::Mat v32f(frame.height, frame.width, CV_32FC1);
        cv::Mat uv_image(frame.height, frame.width, CV_32FC2);

        u16u.convertTo(u32f, CV_32FC1, u_magnification, u_bias);
        v16u.convertTo(v32f, CV_32FC1, v_magnification, v_bias);

        cv::Mat in_uv[] = {u32f, v32f};
        int from_to_uv[] = {0, 0, 1, 1};
        mixChannels(in_uv, 2, &uv_image, 1, from_to_uv, 2);

        std::shared_ptr<rs2::texture_coordinate> texture_coordinates_tmp(
            new rs2::texture_coordinate[frame.n_points],
            std::default_delete<rs2::texture_coordinate[]>());
        frame.texture_coordinates = texture_coordinates_tmp;
        memcpy(frame.texture_coordinates.get(), uv_image.data,
               sizeof(rs2::texture_coordinate) * frame.n_points);
    }

    return frame;
}

int connector_main_loop(
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePushViewer
        &frame_push,
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePopViewer
        &frame_pop,
    int socket) {

    const int BUF_LEN = 50000000;
    char *rec_buf = (char *)malloc(BUF_LEN);
    char *rec_buf2 = (char *)malloc(BUF_LEN);
    char *snd_buf = (char *)malloc(BUF_LEN);
    struct pollfd fd;
    fd.fd = socket;
    fd.events = POLLIN | POLLERR;
    int n_accumlated_read = 0;
    int send_frame_count = 0;

    std::chrono::system_clock::time_point start, end;

    while (1) {
        poll(&fd, 1, 1);
        if (fd.revents & POLLIN) {
            int len_read = 0;
            // If socket == -1 then debug mode.
            if (socket != -1)
                len_read = read(socket, rec_buf + n_accumlated_read, BUF_LEN);
            if (len_read == 0) {
                BOOST_LOG_TRIVIAL(fatal)
                    << "Connection down" << boost::stacktrace::stacktrace();
                std::terminate();
            }
            BOOST_LOG_TRIVIAL(info) << "len_read = " << len_read;
            n_accumlated_read += len_read;
            if (n_accumlated_read > 4) {
                int frame_length = *((uint32_t *)rec_buf);
                BOOST_LOG_TRIVIAL(info)
                    << "frame_length = " << frame_length
                    << ", n_accumlated_read = " << n_accumlated_read;
                if (n_accumlated_read >= frame_length) {
                    start = std::chrono::system_clock::now();

                    auto f = deserialize_frame_data(rec_buf);
                    frame_push.push(f);
                    n_accumlated_read -= frame_length;
                    memcpy(rec_buf2, rec_buf + frame_length, n_accumlated_read);
                    std::swap(rec_buf, rec_buf2);

                    end = std::chrono::system_clock::now();
                    double time = static_cast<double>(
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            end - start)
                            .count() /
                        1000.0);
                    BOOST_LOG_TRIVIAL(info)
                        << "Frame decompression time = " << time << "[ms]";
                }
            }
        }
        if (!frame_pop.empty()) {
            auto f = frame_pop.pop();

            if (send_frame_count % 5 == 0) {
                size_t frame_data_length = serialize_frame_data(*f, snd_buf);
                BOOST_LOG_TRIVIAL(info)
                    << "predicted bps = "
                    << frame_data_length * camera::FPS * 8 / 1024.0 / 1024.0;

                if (socket != -1) {
                    int len_send = send(socket, snd_buf, frame_data_length, 0);
                    BOOST_LOG_TRIVIAL(info)
                        << "len_send = " << frame_data_length;
                }
            }
            send_frame_count++;
        }
    }
    free(rec_buf);
    free(rec_buf2);
    free(snd_buf);
}
} // namespace connector
