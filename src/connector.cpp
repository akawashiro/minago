#include "connector.h"

namespace connector {

int connector_main_loop(
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePushViewer
        &frame_push,
    ThreadSafeQueue<camera::rs2_frame_data>::ThreadSafeQueuePopViewer
        &frame_pop) {
    while (1) {
        if (!frame_pop.empty()) {
            auto f = frame_pop.pop();
            frame_push.push(*f);
        }
    }
}
} // namespace connector
