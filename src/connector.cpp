#include "connector.h"

#include "compress.h"

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
    cv::Mat rgb_image(frame.height, frame.width, CV_8UC3, frame.rgb.get());
    std::shared_ptr<cv::Mat> r8u = std::make_shared<cv::Mat>(
        cv::Mat(rgb_image.rows, rgb_image.cols, CV_8UC1));
    std::shared_ptr<cv::Mat> g8u = std::make_shared<cv::Mat>(
        cv::Mat(rgb_image.rows, rgb_image.cols, CV_8UC1));
    std::shared_ptr<cv::Mat> b8u = std::make_shared<cv::Mat>(
        cv::Mat(rgb_image.rows, rgb_image.cols, CV_8UC1));

    cv::Mat out_rgb[] = {*r8u, *g8u, *b8u};
    int from_to_rgb[] = {0, 0, 1, 1, 2, 2};
    mixChannels(&rgb_image, 1, out_rgb, 3, from_to_rgb, 3);

    // std::shared_ptr<cv::Mat> rr = residue_of_history(r8u_history, r8u);
    compress((char *)(*r8u).data, frame.height * frame.width * sizeof(uint8_t),
             compress_output, &compress_length);
    std::cout << "red: original size = "
              << frame.height * frame.width * sizeof(uint8_t)
              << ", compressed size = " << compress_length << std::endl;
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    // std::shared_ptr<cv::Mat> gr = residue_of_history(g8u_history, g8u);
    compress((char *)(*g8u).data, frame.height * frame.width * sizeof(uint8_t),
             compress_output, &compress_length);
    std::cout << "green: original size = "
              << frame.height * frame.width * sizeof(uint8_t)
              << ", compressed size = " << compress_length << std::endl;
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    // std::shared_ptr<cv::Mat> br = residue_of_history(b8u_history, b8u);
    compress((char *)(*b8u).data, frame.height * frame.width * sizeof(uint8_t),
             compress_output, &compress_length);
    std::cout << "blue: original size = "
              << frame.height * frame.width * sizeof(uint8_t)
              << ", compressed size = " << compress_length << std::endl;
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    // XYZ
    cv::Mat xyz_image(frame.height, frame.width, CV_32FC3,
                      frame.vertices.get());
    cv::Mat x32f(rgb_image.rows, rgb_image.cols, CV_32FC1);
    cv::Mat y32f(rgb_image.rows, rgb_image.cols, CV_32FC1);
    cv::Mat z32f(rgb_image.rows, rgb_image.cols, CV_32FC1);

    cv::Mat out_xyz[] = {x32f, y32f, z32f};
    int from_to_xyz[] = {0, 0, 1, 1, 2, 2};
    mixChannels(&xyz_image, 1, out_xyz, 3, from_to_xyz, 3);

    // Normalization
    double max_x, min_x, max_y, min_y, max_z, min_z;
    cv::minMaxLoc(x32f, &min_x, &max_x);
    cv::minMaxLoc(y32f, &min_y, &max_y);
    cv::minMaxLoc(z32f, &min_z, &max_z);
    // min_x = min_y = min_z = -50;
    // max_x = max_y = max_z = 50;

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
    std::cout << "x: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;

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
    std::cout << "y: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;

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
    std::cout << "z: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
    *((float *)p) = (float)((max_z - min_z) / ABS_MAX_16SU);
    p += sizeof(float);
    *((float *)p) = (float)min_z;
    p += sizeof(float);
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    // UV
    cv::Mat uv_image(frame.height, frame.width, CV_32FC2,
                     frame.texture_coordinates.get());
    cv::Mat u32f(rgb_image.rows, rgb_image.cols, CV_32FC1);
    cv::Mat v32f(rgb_image.rows, rgb_image.cols, CV_32FC1);

    cv::Mat out_uv[] = {u32f, v32f};
    int from_to_uv[] = {0, 0, 1, 1};
    mixChannels(&uv_image, 1, out_uv, 2, from_to_uv, 2);

    // Normalization
    double max_u, min_u, max_v, min_v;
    cv::minMaxLoc(u32f, &min_u, &max_u);
    cv::minMaxLoc(v32f, &min_v, &max_v);
    // min_u = min_v = -2;
    // max_u = max_v = 2;

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
    std::cout << "u: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
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
    std::cout << "v: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
    *((float *)p) = (float)((max_v - min_v) / frame.height);
    p += sizeof(float);
    *((float *)p) = (float)min_v;
    p += sizeof(float);
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    // Write the size of compressed frame to the head.
    *((uint32_t *)buf) = p - buf;

    free(compress_output);

    return p - buf;
}

camera::rs2_frame_data deserialize_frame_data(char *buf) {
    camera::rs2_frame_data frame;
    char *p = buf;

    // Skip the first 4 bytes of the length of the serialized data.
    p += sizeof(uint32_t);

    frame.height = *((uint32_t *)p);
    p += sizeof(uint32_t);

    frame.width = *((uint32_t *)p);
    p += sizeof(uint32_t);

    frame.n_points = *((uint32_t *)p);
    p += sizeof(uint32_t);

    std::shared_ptr<uint8_t[]> rgb_tmp(
        new uint8_t[3 * frame.width * frame.height]);
    frame.rgb = rgb_tmp;
    memcpy(frame.rgb.get(), p,
           sizeof(uint8_t) * 3 * frame.width * frame.height);
    p += sizeof(uint8_t) * 3 * frame.width * frame.height;

    std::shared_ptr<rs2::vertex[]> vertices_tmp(
        new rs2::vertex[frame.n_points]);
    frame.vertices = vertices_tmp;
    memcpy(frame.vertices.get(), p, sizeof(rs2::vertex) * frame.n_points);
    p += sizeof(rs2::vertex) * frame.n_points;

    std::shared_ptr<rs2::texture_coordinate[]> texture_coordinates_tmp(
        new rs2::texture_coordinate[frame.n_points]);
    frame.texture_coordinates = texture_coordinates_tmp;
    memcpy(frame.texture_coordinates.get(), p,
           sizeof(rs2::texture_coordinate) * frame.n_points);

    return frame;
}

int connector_main_loop(
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePushViewer
        &frame_push,
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePopViewer
        &frame_pop,
    int socket) {

    const int BUF_LEN = 50000000;
    char *buf = (char *)malloc(BUF_LEN);
    char *buf2 = (char *)malloc(BUF_LEN);
    struct pollfd fd;
    fd.fd = socket;
    fd.events = POLLIN | POLLERR;
    int n_accumlated_read = 0;

    while (1) {
        poll(&fd, 1, 1);
        if (fd.revents & POLLIN) {
            int len_read = 0;
            // If socket == -1 then debug mode.
            if (socket != -1)
            len_read = read(socket, buf + n_accumlated_read, BUF_LEN);
            std::cout << "len_read = " << len_read << std::endl;
            n_accumlated_read += len_read;
            if (n_accumlated_read > 4) {
                int frame_length = *((uint32_t *)buf);
                std::cout << "frame_length = " << frame_length
                          << ", n_accumlated_read = " << n_accumlated_read
                          << std::endl;
                if (n_accumlated_read >= frame_length) {
                    auto f = deserialize_frame_data(buf);
                    frame_push.push(f);
                    n_accumlated_read -= frame_length;
                    memcpy(buf2, buf + frame_length, n_accumlated_read);
                    std::swap(buf, buf2);
                }
            }
        }
        if (!frame_pop.empty()) {
            auto f = frame_pop.pop();

            size_t len =
                (*f).n_points * (sizeof(uint8_t) * 3 + sizeof(rs2::vertex) +
                                 sizeof(rs2::texture_coordinate));
            char *compress_frame = (char *)malloc(len);
            size_t len2 = serialize_frame_data(*f, compress_frame);
            free(compress_frame);

            std::cout << "length_of_serialize_data(*f) = " << len << std::endl;
            std::cout << "length of compressed frame = " << len2 << std::endl;
            std::cout << "predicted bps = " << len2 * 25 * 8 / 1024.0 / 1024.0
                      << std::endl;

            serialize_frame_data(*f, buf);
            if (socket != -1) {
            int len_send = send(socket, buf, len, 0);
            std::cout << "len_send = " << len_send << std::endl;
        }
    }
}
} // namespace connector
