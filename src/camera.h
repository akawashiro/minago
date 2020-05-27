#pragma once

#include <librealsense2/rs.hpp>

#include "eyeLike.h"
#include "thread_safe_queue.h"

namespace camera {

struct rs2_frame_data {
    rs2::depth_frame depth;
    rs2::video_frame color;
};

int camera_main_loop(
    ThreadSafeQueuePushViewer<eye_like::EyesPosition> &pos_queue,
    ThreadSafeQueuePushViewer<rs2_frame_data> &frame_queue);
} // namespace camera
