#pragma once

#include <librealsense2/rs.hpp>

#include "camera.h"
#include "eyeLike.h"
#include "thread_safe_queue.h"

namespace connector {

int connector_main_loop(
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePushViewer
        &frame_push,
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePopViewer
        &frame_pop);
} // namespace connector
