#include <iostream>
#include <thread>

#include "camera.h"
#include "connector.h"
#include "renderer.h"

int main() {
    int connection_type;
    std::cout << "Connection type (1: server / 2: client) > ";
    std::cin >> connection_type;
    if (connection_type != 1 && connection_type != 2) {
        std::cout << "Invalid connection type: " << connection_type
                  << std::endl;
        return 0;
    }

    ThreadSafeState<eye_like::EyesPosition> eye_pos;
    auto eye_pos_get = eye_pos.getGetView();
    auto eye_pos_put = eye_pos.getPutView();

    ThreadSafeQueue<camera::rs2_frame_data> frame_camera_connector;
    auto frame_camera_connector_push = frame_camera_connector.getPushView();
    auto frame_camera_connector_pop = frame_camera_connector.getPopView();

    ThreadSafeQueue<camera::rs2_frame_data> frame_connector_renderer;
    auto frame_connector_renderer_push = frame_connector_renderer.getPushView();
    auto frame_connector_renderer_pop = frame_connector_renderer.getPopView();

    std::thread th_camera(camera::camera_main_loop, std::ref(eye_pos_put),
                          std::ref(frame_camera_connector_push));
    std::thread th_renderer(renderer::renderer_main_loop, std::ref(eye_pos_get),
                            std::ref(frame_connector_renderer_pop));
    std::thread th_connector(connector::connector_main_loop,
                             std::ref(frame_connector_renderer_push),
                             std::ref(frame_camera_connector_pop));

    th_camera.join();
    th_renderer.join();
    return 0;
}