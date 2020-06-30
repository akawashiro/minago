#include "camera.h"

int main(int argc, char *argv[]) {
    // Initialize Google's logging library.
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, true);

    ThreadSafeState<eye_like::EyesPosition> eye_pos;
    auto eye_pos_put = eye_pos.getPutView();
    ThreadSafeQueue<camera::rs2_frame_data> frame_queue;
    auto frame_queue_push = frame_queue.getPushView();
    camera::camera_main_loop(eye_pos_put, frame_queue_push, true, true);
}
