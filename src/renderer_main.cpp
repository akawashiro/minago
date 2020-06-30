#include "renderer.h"

int main(int argc, char *argv[]) {
    // Initialize Google's logging library.
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, true);

    ThreadSafeState<eye_like::EyesPosition> eye_pos;
    auto eye_pos_get = eye_pos.getGetView();
    ThreadSafeQueue<camera::rs2_frame_data> frame_queue;
    auto frame_queue_pop = frame_queue.getPopView();
    renderer::renderer_main_loop(eye_pos_get, frame_queue_pop, true);
}
