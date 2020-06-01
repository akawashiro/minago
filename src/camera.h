#pragma once

#include <atomic>
#include <librealsense2/rs.hpp>

#include "eyeLike.h"
#include "thread_safe_queue.h"

namespace camera {

struct rs2_frame_data {
    uint32_t height, width, n_points;
    std::shared_ptr<uint8_t[]> rgb;
    std::shared_ptr<rs2::vertex[]> vertices;
    std::shared_ptr<rs2::texture_coordinate[]> texture_coordinates;
};

int camera_main_loop(
    ThreadSafeState<eye_like::EyesPosition>::ThreadSafeStatePutViewer &eye_pos,
    ThreadSafeQueue<rs2_frame_data>::ThreadSafeQueuePushViewer &frame_queue);
} // namespace camera
