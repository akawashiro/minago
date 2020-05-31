#pragma once

#include <atomic>
#include <librealsense2/rs.hpp>

#include "eyeLike.h"
#include "thread_safe_queue.h"

namespace camera {

struct rs2_frame_data {
    rs2::depth_frame depth;
    rs2::video_frame color;
};

int camera_main_loop(
    ThreadSafeState<eye_like::EyesPosition>::ThreadSafeStatePutViewer &eye_pos,
    ThreadSafeQueue<rs2_frame_data>::ThreadSafeQueuePushViewer &frame_queue);
} // namespace camera
