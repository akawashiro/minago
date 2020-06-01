#include "camera.h"

int main() {
    ThreadSafeState<eye_like::EyesPosition> eye_pos;
    auto eye_pos_put = eye_pos.getPutView();
    ThreadSafeQueue<camera::rs2_frame_data> frame_queue;
    auto frame_queue_push = frame_queue.getPushView();
    camera::camera_main_loop(eye_pos_put, frame_queue_push, false);
}