#include "camera.h"
#include "eyeLike.h"
#include "thread_safe_queue.h"

namespace renderer {
int renderer_main_loop(
    ThreadSafeQueuePopViewer<eye_like::EyesPosition> &pos_queue,
    ThreadSafeQueuePopViewer<camera::rs2_frame_data> &frame_queue);
}