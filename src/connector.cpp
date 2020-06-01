#include "connector.h"

#include <arpa/inet.h>
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
            std::cout << "length_of_serialize_data(*f) = " << len << std::endl;
            serialize_frame_data(*f, buf);
            int len_send = send(socket, buf, len, 0);
            std::cout << "len_send = " << len_send << std::endl;
        }
    }
}
} // namespace connector
