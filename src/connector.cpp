#include "connector.h"

#include "compress.h"

#include <arpa/inet.h>
#include <cassert>
#include <deque>
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

const int MAX_HISTORY_LENGTH = 2;
const double ABS_MAX_16SU = 10000.0;

std::deque<std::shared_ptr<cv::Mat>> r8u_history;
std::deque<std::shared_ptr<cv::Mat>> g8u_history;
std::deque<std::shared_ptr<cv::Mat>> b8u_history;
std::deque<std::shared_ptr<cv::Mat>> x16u_history;
std::deque<std::shared_ptr<cv::Mat>> y16u_history;
std::deque<std::shared_ptr<cv::Mat>> z16u_history;
std::deque<std::shared_ptr<cv::Mat>> u16u_history;
std::deque<std::shared_ptr<cv::Mat>> v16u_history;

template <typename T>
void push_until_length(std::deque<T> &q, const T &v, int length) {
    q.push_front(v);
    if (q.size() > length)
        q.pop_back();
}

std::shared_ptr<cv::Mat>
residue_of_history(const std::deque<std::shared_ptr<cv::Mat>> &history,
                   const std::shared_ptr<cv::Mat> &data) {
    int n = history.size();

    if (n == 0) {
        return std::make_shared<cv::Mat>((*data).clone());
    } else if (n == 1) {
        std::shared_ptr<cv::Mat> ret =
            std::make_shared<cv::Mat>(cv::Mat(*data - (*(history[0]))));
        return ret;
    } else if (n == 2) {
        std::shared_ptr<cv::Mat> ret =
            std::make_shared<cv::Mat>((cv::Mat(*data - (*(history[0])))) -
                                      (*(history[0]) - *(history[1])));
        return ret;
    } else {
        return std::make_shared<cv::Mat>((*data).clone());
    }
}

std::shared_ptr<cv::Mat>
restore_with_history(const std::deque<std::shared_ptr<cv::Mat>> &history,
                     const std::shared_ptr<cv::Mat> &data) {
    int n = history.size();

    if (n == 0) {
        return std::make_shared<cv::Mat>((*data).clone());
    } else if (n == 1) {
        std::shared_ptr<cv::Mat> ret =
            std::make_shared<cv::Mat>(cv::Mat(*data + (*(history[0]))));
        return ret;
    } else if (n == 2) {
        std::shared_ptr<cv::Mat> ret =
            std::make_shared<cv::Mat>((cv::Mat(*data + (*(history[0])))) +
                                      (*(history[0]) - *(history[1])));
        return ret;
    } else {
        return std::make_shared<cv::Mat>((*data).clone());
    }
}

void pixel_lost_check(const std::deque<std::shared_ptr<cv::Mat>> &history,
                      const std::shared_ptr<cv::Mat> &compressed,
                      const std::shared_ptr<cv::Mat> &original, int line) {
    std::shared_ptr<cv::Mat> restored =
        restore_with_history(history, compressed);
    cv::Mat comp;
    cv::compare(*original, *restored, comp, cv::CMP_EQ);
    int n_lost = (*original).rows * (*original).cols - cv::countNonZero(comp);
    if (n_lost != 0) {
        std::cout << "line: " << line << ": " << n_lost << " pixels are lost."
                  << std::endl;
        assert(n_lost == 0);
    }
}

// TODO
// void decompress_check(const char* compress, int compress_length){
//     char *decompress_output =
//         (char *)malloc(1280 * 720 * sizeof(rs2::vertex));
// }

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
    min_x = min_y = min_z = -50;
    max_x = max_y = max_z = 50;

    x32f = x32f - min_x;
    y32f = y32f - min_y;
    z32f = z32f - min_z;

    std::shared_ptr<cv::Mat> x16u =
        std::make_shared<cv::Mat>(cv::Mat(x32f.rows, x32f.cols, CV_16SC1));
    std::shared_ptr<cv::Mat> y16u =
        std::make_shared<cv::Mat>(cv::Mat(y32f.rows, y32f.cols, CV_16SC1));
    std::shared_ptr<cv::Mat> z16u =
        std::make_shared<cv::Mat>(cv::Mat(z32f.rows, z32f.cols, CV_16SC1));
    x32f.convertTo(*x16u, CV_16SC3, ABS_MAX_16SU / (max_x - min_x));
    y32f.convertTo(*y16u, CV_16SC3, ABS_MAX_16SU / (max_y - min_y));
    z32f.convertTo(*z16u, CV_16SC3, ABS_MAX_16SU / (max_z - min_z));

    std::shared_ptr<cv::Mat> xr = residue_of_history(x16u_history, x16u);
    pixel_lost_check(x16u_history, xr, x16u, __LINE__);

    compress((char *)(*xr).data, frame.height * frame.width * sizeof(uint16_t),
             compress_output, &compress_length);
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

    std::shared_ptr<cv::Mat> yr = residue_of_history(y16u_history, y16u);
    pixel_lost_check(y16u_history, yr, y16u, __LINE__);

    compress((char *)(*yr).data, frame.height * frame.width * sizeof(uint16_t),
             compress_output, &compress_length);
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

    std::shared_ptr<cv::Mat> zr = residue_of_history(z16u_history, z16u);
    pixel_lost_check(z16u_history, zr, z16u, __LINE__);

    compress((char *)(*zr).data, frame.height * frame.width * sizeof(uint16_t),
             compress_output, &compress_length);
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
    min_u = min_v = -2;
    max_u = max_v = 2;

    u32f = u32f - min_u;
    v32f = v32f - min_v;

    std::shared_ptr<cv::Mat> u16u =
        std::make_shared<cv::Mat>(cv::Mat(u32f.rows, u32f.cols, CV_16SC1));
    std::shared_ptr<cv::Mat> v16u =
        std::make_shared<cv::Mat>(cv::Mat(v32f.rows, v32f.cols, CV_16SC1));
    u32f.convertTo(*u16u, CV_16SC3, ABS_MAX_16SU / (max_u - min_u));
    v32f.convertTo(*v16u, CV_16SC3, ABS_MAX_16SU / (max_v - min_v));

    std::shared_ptr<cv::Mat> ur = residue_of_history(u16u_history, u16u);
    pixel_lost_check(u16u_history, ur, u16u, __LINE__);

    compress((char *)(*ur).data, frame.height * frame.width * sizeof(uint16_t),
             compress_output, &compress_length);
    std::cout << "u: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
    *((float *)p) = (float)((max_u - min_u) / ABS_MAX_16SU);
    p += sizeof(float);
    *((float *)p) = (float)min_u;
    p += sizeof(float);
    *((uint32_t *)p) = compress_length;
    p += sizeof(uint32_t);
    memcpy(p, compress_output, compress_length);
    p += compress_length;

    std::shared_ptr<cv::Mat> vr = residue_of_history(v16u_history, v16u);
    pixel_lost_check(v16u_history, vr, v16u, __LINE__);

    compress((char *)(*vr).data, frame.height * frame.width * sizeof(uint16_t),
             compress_output, &compress_length);
    std::cout << "v: original size = "
              << frame.height * frame.width * sizeof(uint16_t)
              << ", compressed size = " << compress_length << std::endl;
    *((float *)p) = (float)((max_v - min_v) / ABS_MAX_16SU);
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

    push_until_length(r8u_history, r8u, MAX_HISTORY_LENGTH);
    push_until_length(g8u_history, g8u, MAX_HISTORY_LENGTH);
    push_until_length(b8u_history, b8u, MAX_HISTORY_LENGTH);
    push_until_length(x16u_history, x16u, MAX_HISTORY_LENGTH);
    push_until_length(y16u_history, y16u, MAX_HISTORY_LENGTH);
    push_until_length(z16u_history, z16u, MAX_HISTORY_LENGTH);
    push_until_length(u16u_history, u16u, MAX_HISTORY_LENGTH);
    push_until_length(v16u_history, v16u, MAX_HISTORY_LENGTH);

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
