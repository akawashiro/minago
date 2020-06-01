#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "camera.h"
#include "connector.h"
#include "renderer.h"

#define PORT 8080

int setup_client() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    send(sock, hello, strlen(hello), 0);
    printf("Hello message sent\n");
    valread = read(sock, buffer, 1024);
    printf("%s\n", buffer);
    return sock;
}

int setup_server() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "Hello from server";
    struct pollfd fd;

    memset(&fd, 0, sizeof(fd));

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    fd.fd = new_socket;
    fd.events = POLLIN | POLLERR;

    while (1) {
        poll(&fd, 1, 10);
        if (fd.revents & POLLIN) {
            valread = read(new_socket, buffer, 1024);
            printf("%s\n", buffer);
            send(new_socket, hello, strlen(hello), 0);
            printf("Hello message sent\n");
            break;
        }
    }
    return new_socket;
}

int main() {
    int socket = -1;
    int use_realsense;
    int connection_type;

    std::cout << "Realsense or webcam (1: realsense / 2: webcam) > ";
    std::cin >> use_realsense;
    if (use_realsense != 1 && use_realsense != 2) {
        std::cout << "Invalid input: " << use_realsense << std::endl;
        return 0;
    } else {
        if (use_realsense == 1) {
            use_realsense = true;
        } else {
            use_realsense = false;
        }
    }

    std::cout << "Connection type (1: server / 2: client / 3: debug) > ";
    std::cin >> connection_type;
    if (connection_type != 1 && connection_type != 2 && connection_type != 3) {
        std::cout << "Invalid connection type: " << connection_type
                  << std::endl;
        return 0;
    } else if (connection_type == 1) {
        socket = setup_server();
    } else if (connection_type == 2) {
        socket = setup_client();
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
                          std::ref(frame_camera_connector_push), use_realsense);
    std::thread th_renderer(renderer::renderer_main_loop, std::ref(eye_pos_get),
                            std::ref(frame_connector_renderer_pop));
    std::thread th_connector(connector::connector_main_loop,
                             std::ref(frame_connector_renderer_push),
                             std::ref(frame_camera_connector_pop), socket);

    th_camera.join();
    th_renderer.join();
    return 0;
}