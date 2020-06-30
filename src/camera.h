#pragma once

#include <atomic>
#include <string>

#include <glog/logging.h>
#include <librealsense2/rs.hpp>

#include "eyeLike.h"
#include "thread_safe_queue.h"

namespace camera {

const int FPS = 15;
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 360;
const int FRAME_DUMP_MAX =
    sizeof(uint32_t) * 4 + FRAME_HEIGHT * FRAME_WIDTH * 3 * sizeof(uint8_t) +
    FRAME_HEIGHT * FRAME_WIDTH * sizeof(rs2::vertex) +
    FRAME_HEIGHT * FRAME_WIDTH * sizeof(rs2::texture_coordinate);
const std::string realsense_frame_dump_file = "../misc/realsense_frame_dump";

struct rs2_frame_data {
    uint32_t height, width, n_points;
    std::shared_ptr<uint8_t> rgb;
    std::shared_ptr<rs2::vertex> vertices;
    std::shared_ptr<rs2::texture_coordinate> texture_coordinates;
};

void save_frame(rs2_frame_data frame, const std::string &path);

rs2_frame_data read_frame(const std::string &path);

int camera_main_loop(
    ThreadSafeState<eye_like::EyesPosition>::ThreadSafeStatePutViewer &eye_pos,
    ThreadSafeQueue<rs2_frame_data>::ThreadSafeQueuePushViewer &frame_queue,
    bool use_realsense, bool debug);
} // namespace camera
