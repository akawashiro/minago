#include "connector.h"

#include "compress.h"

#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

namespace connector {

size_t length_of_serialize_data(camera::rs2_frame_data frame) {
    // the first 4 bytes of serialized data is the length.
    return sizeof(uint32_t) * 4 +
           frame.height * frame.width * 3 * sizeof(uint8_t) +
           frame.n_points * sizeof(rs2::vertex) +
           frame.n_points * sizeof(rs2::texture_coordinate);
}

void serialize_frame_data(camera::rs2_frame_data frame, char *buf) {
    char *p = buf;

    *((uint32_t *)p) = (uint32_t)length_of_serialize_data(frame);
    p += sizeof(uint32_t);

    *((uint32_t *)p) = frame.height;
    p += sizeof(uint32_t);

    *((uint32_t *)p) = frame.width;
    p += sizeof(uint32_t);

    *((uint32_t *)p) = frame.n_points;
    p += sizeof(uint32_t);

    memcpy(p, frame.rgb.get(),
           frame.height * frame.width * 3 * sizeof(uint8_t));
    p += frame.height * frame.width * 3 * sizeof(uint8_t);

    memcpy(p, frame.vertices.get(), frame.n_points * sizeof(rs2::vertex));
    p += frame.n_points * sizeof(rs2::vertex);

    memcpy(p, frame.texture_coordinates.get(),
           frame.n_points * sizeof(rs2::texture_coordinate));
}

cv::Mat x16u_old;
cv::Mat y16u_old;
cv::Mat z16u_old;

uint32_t serialize_frame_data_2(camera::rs2_frame_data frame, char *buf) {
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
    cv::Mat r8(rgb_image.rows, rgb_image.cols, CV_8UC1);
    cv::Mat g8(rgb_image.rows, rgb_image.cols, CV_8UC1);
    cv::Mat b8(rgb_image.rows, rgb_image.cols, CV_8UC1);

    cv::Mat out_rgb[] = {r8, g8, b8};
    int from_to_rgb[] = {0, 0, 1, 1, 2, 2};
    mixChannels(&rgb_image, 1, out_rgb, 3, from_to_rgb, 3);

    compress((char *)r8.data, frame.height * frame.width * sizeof(uint8_t),
             compress_output, &compress_length);
    std::cout << "red: original size = "
              << frame.height * frame.width * sizeof(uint8_t)
              << ", compressed size = " << compress_length << std::endl;
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    compress((char *)g8.data, frame.height * frame.width * sizeof(uint8_t),
             compress_output, &compress_length);
    std::cout << "green: original size = "
              << frame.height * frame.width * sizeof(uint8_t)
              << ", compressed size = " << compress_length << std::endl;
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    compress((char *)b8.data, frame.height * frame.width * sizeof(uint8_t),
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
    x32f = x32f - min_x;
    y32f = y32f - min_y;
    z32f = z32f - min_z;

    cv::Mat x16u(x32f.rows, x32f.cols, CV_16UC1);
    cv::Mat y16u(y32f.rows, y32f.cols, CV_16UC1);
    cv::Mat z16u(z32f.rows, z32f.cols, CV_16UC1);
    x32f.convertTo(x16u, CV_16UC3, 65535.0 / (max_x - min_x));
    y32f.convertTo(y16u, CV_16UC3, 65535.0 / (max_y - min_y));
    z32f.convertTo(z16u, CV_16UC3, 65535.0 / (max_z - min_z));

    if (!x16u_old.empty()) {
        cv::Mat xr = x16u - x16u_old;
        // for (int y = 0; y < frame.height; y++) {
        //     for (int x = 0; x < frame.width; x++) {
        //         std::cout << xr.at<uint16_t>(y, x) << " ";
        //     }
        //     std::cout << std::endl;
        // }
        compress((char *)xr.data, frame.height * frame.width * sizeof(uint16_t),
                 compress_output, &compress_length);
    } else {
        compress((char *)x16u.data,
                 frame.height * frame.width * sizeof(uint16_t), compress_output,
                 &compress_length);
    }
    x16u_old = x16u;

    // compress((char *)x16u.data, frame.height * frame.width *
    // sizeof(uint16_t),
    //          compress_output, &compress_length);
    std::cout << "x: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
    *((float *)p) = (float)((max_x - min_x) / 65535.0);
    p += sizeof(float);
    *((float *)p) = (float)min_x;
    p += sizeof(float);
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    compress((char *)y16u.data, frame.height * frame.width * sizeof(uint16_t),
             compress_output, &compress_length);
    std::cout << "y: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
    *((float *)p) = (float)((max_y - min_y) / 65535.0);
    p += sizeof(float);
    *((float *)p) = (float)min_y;
    p += sizeof(float);
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    compress((char *)z16u.data, frame.height * frame.width * sizeof(uint16_t),
             compress_output, &compress_length);
    std::cout << "z: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
    *((float *)p) = (float)((max_z - min_z) / 65535.0);
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
    u32f = u32f - min_u;
    v32f = v32f - min_v;

    cv::Mat u16u(u32f.rows, u32f.cols, CV_16UC1);
    cv::Mat v16u(v32f.rows, v32f.cols, CV_16UC1);
    u32f.convertTo(u16u, CV_16UC3, 65535.0 / (max_u - min_u));
    v32f.convertTo(v16u, CV_16UC3, 65535.0 / (max_v - min_v));

    compress((char *)u16u.data, frame.height * frame.width * sizeof(uint16_t),
             compress_output, &compress_length);
    std::cout << "u: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
    *((float *)p) = (float)((max_u - min_u) / 65535.0);
    p += sizeof(float);
    *((float *)p) = (float)min_u;
    p += sizeof(float);
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    compress((char *)v16u.data, frame.height * frame.width * sizeof(uint16_t),
             compress_output, &compress_length);
    std::cout << "v: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
    *((float *)p) = (float)((max_v - min_v) / 65535.0);
    p += sizeof(float);
    *((float *)p) = (float)min_v;
    p += sizeof(float);
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

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
            int len_read;
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

            size_t len = length_of_serialize_data(*f);
            char *compress_frame = (char *)malloc(len);
            size_t len2 = serialize_frame_data_2(*f, compress_frame);
            free(compress_frame);

            std::cout << "length_of_serialize_data(*f) = " << len << std::endl;
            std::cout << "length of compressed frame = " << len2 << std::endl;

            serialize_frame_data(*f, buf);
            int len_send = send(socket, buf, len, 0);
            std::cout << "len_send = " << len_send << std::endl;
        }
    }
}
} // namespace connector
