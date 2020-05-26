#include "eyeLike.h"
#include "thread_safe_queue.h"

namespace camera {
int camera_main_loop(
    ThreadSafeQueuePushViewer<eye_like::EyesPosition> &pos_queue);
}
