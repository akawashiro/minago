#include "camera.h"

int main() {
    ThreadSafeQueue<eye_like::EyesPosition> eye_queue;
    ThreadSafeQueue<camera::rs2_frame_data> frame_queue;
    auto eye_queue_push = ThreadSafeQueuePushViewer(eye_queue);
    auto frame_queue_push = ThreadSafeQueuePushViewer(frame_queue);
    camera::camera_main_loop(eye_queue_push, frame_queue_push);
}