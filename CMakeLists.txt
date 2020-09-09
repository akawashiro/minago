# CMake entry point
cmake_minimum_required(VERSION 3.0)
project(minago)

find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(ZLIB REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_BINARY_DIR ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 17)

# Compile external dependencies
add_subdirectory(external)

# On Visual 2005 and above, this module can set the debug working directory
cmake_policy(SET CMP0026 OLD)
list(APPEND CMAKE_MODULE_PATH
     "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(CreateLaunchers)

include_directories(
  external/glfw-3.1.2/include/ external/glm-0.9.7.1/
  external/glew-1.13.0/include/ external/librealsense-2.35.2/include/ .)

set(ALL_LIBS
    ${OPENGL_LIBRARY}
    ${OpenCV_LIBS}
    ${ZLIB_LIBRARIES}
    glfw
    GLEW_1130
    realsense2
    glog)

add_definitions(-DTW_STATIC -DTW_NO_LIB_PRAGMA -DTW_NO_DIRECT3D -DGLEW_STATIC
                -D_CRT_SECURE_NO_WARNINGS)

add_executable(obj-file-loader src/obj_file_loader_main.cpp
                               src/obj_file_loader.cpp)

target_link_libraries(obj-file-loader ${ALL_LIBS})

create_target_launcher(obj-file-loader WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")

add_custom_command(
  TARGET obj-file-loader
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/obj-file-loader${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

add_executable(
  eye-like
  src/eye_like_main.cpp
  src/eye_like.cpp
  src/find_eye_corner.cpp
  src/find_eye_center.cpp
  src/helpers.cpp)

target_link_libraries(eye-like ${ALL_LIBS})

create_target_launcher(eye-like WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")

add_custom_command(
  TARGET eye-like
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/eye-like${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

add_executable(
  3d_telecom
  src/3d_telecom.cpp
  src/obj_file_loader.cpp
  src/eye_like.cpp
  src/find_eye_center.cpp
  src/find_eye_corner.cpp
  src/helpers.cpp
  src/thread_safe_queue.cpp)

target_link_libraries(3d_telecom ${ALL_LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})

create_target_launcher(3d_telecom WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")

add_custom_command(
  TARGET 3d_telecom
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/3d_telecom${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# video_capture_speed
add_executable(video_capture_speed src/video_capture_speed.cpp)
target_link_libraries(video_capture_speed ${ALL_LIBS})
create_target_launcher(video_capture_speed WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET video_capture_speed
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/video_capture_speed${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# rgbd-to-3d
add_executable(
  rgbd-to-3d src/rgbd_to_3d.cpp src/eye_like.cpp src/find_eye_center.cpp
             src/find_eye_corner.cpp src/helpers.cpp)

target_link_libraries(rgbd-to-3d ${ALL_LIBS})
create_target_launcher(rgbd-to-3d WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET rgbd-to-3d
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/rgbd-to-3d${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# camera
add_executable(
  camera
  src/camera_main.cpp
  src/camera.cpp
  src/eye_like.cpp
  src/find_eye_center.cpp
  src/find_eye_corner.cpp
  src/helpers.cpp
  src/thread_safe_queue.cpp)

target_link_libraries(camera ${ALL_LIBS})
create_target_launcher(camera WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET camera
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/camera${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# renderer
add_executable(
  renderer
  src/renderer_main.cpp
  src/renderer.cpp
  src/camera.cpp
  src/eye_like.cpp
  src/find_eye_center.cpp
  src/find_eye_corner.cpp
  src/helpers.cpp
  src/thread_safe_queue.cpp)

target_link_libraries(renderer ${ALL_LIBS})
create_target_launcher(renderer WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET renderer
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/renderer${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# point-cloud-compress
add_executable(
  point-cloud-compress
  src/point-cloud-compress.cpp src/eye_like.cpp src/find_eye_center.cpp
  src/find_eye_corner.cpp src/helpers.cpp src/thread_safe_queue.cpp)

target_link_libraries(point-cloud-compress ${ALL_LIBS})
create_target_launcher(point-cloud-compress WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET point-cloud-compress
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/point-cloud-compress${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# minago
add_executable(
  minago
  src/minago.cpp
  src/camera.cpp
  src/renderer.cpp
  src/eye_like.cpp
  src/find_eye_center.cpp
  src/find_eye_corner.cpp
  src/connector.cpp
  src/helpers.cpp
  src/compress.cpp
  src/thread_safe_queue.cpp)

target_link_libraries(minago ${ALL_LIBS})
create_target_launcher(minago WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET minago
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/minago${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")
