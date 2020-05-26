#include "camera.h"

int main() {
    ThreadSafeQueue<eye_like::EyesPosition> eye_queue;
    auto t = ThreadSafeQueuePushViewer(eye_queue);
    camera::camera_main_loop(t);
}