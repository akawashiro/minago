#include "camera.h"
#include "eye_like.h"
#include "thread_safe_queue.h"

namespace renderer {
int renderer_main_loop(
    ThreadSafeState<eye_like::EyesPosition>::ThreadSafeStateGetViewer
        &eye_pos_get,
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePopViewer
        &frame_queue,
    bool debug);
}
