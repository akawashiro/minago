# CMake entry point
cmake_minimum_required(VERSION 3.0)
project(minago)

find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(ZLIB REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_BINARY_DIR ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 20)

# Compile external dependencies
add_subdirectory(external)

# On Visual 2005 and above, this module can set the debug working directory
cmake_policy(SET CMP0026 OLD)
list(APPEND CMAKE_MODULE_PATH
     "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(CreateLaunchers)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(third-party)
get_third_party(glog)
set(WITH_GFLAGS
    OFF
    CACHE INTERNAL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/glog
                 ${CMAKE_CURRENT_BINARY_DIR}/glog EXCLUDE_FROM_ALL)
add_definitions(-DC10_USE_GLOG=1)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/glog
  ${CMAKE_CURRENT_BINARY_DIR}/glog/src
  ${OpenCV_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw-3.1.2/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/external/glm-0.9.7.1/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/external/glew-1.13.0/include/
  ${CMAKE_CURRENT_SOURCE_DIR}/external/librealsense-2.35.2/include/)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/external/librealsense-2.35.2)

add_definitions(-DTW_STATIC -DTW_NO_LIB_PRAGMA -DTW_NO_DIRECT3D -DGLEW_STATIC
                -D_CRT_SECURE_NO_WARNINGS)

# ========== obj-file-loader ==========
add_library(obj-file-loader-lib src/obj_file_loader.cpp)
add_executable(obj-file-loader src/obj_file_loader_main.cpp)
target_link_libraries(obj-file-loader obj-file-loader-lib ${OPENGL_LIBRARY}
                      glfw glog)
create_target_launcher(obj-file-loader WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET obj-file-loader
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/obj-file-loader${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# ========== eye-like ==========
add_library(eye-like-lib src/eye_like.cpp src/find_eye_corner.cpp
                         src/find_eye_center.cpp src/helpers.cpp)
add_executable(eye-like src/eye_like_main.cpp)
target_link_libraries(eye-like eye-like-lib ${OpenCV_LIBS})
create_target_launcher(eye-like WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET eye-like
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/eye-like${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# ========== thread-safe-queue ==========
add_library(thread-safe-queue-lib src/thread_safe_queue.cpp)

# ========== 3d-telecom ==========
add_executable(3d-telecom src/3d_telecom.cpp)
target_link_libraries(
  3d-telecom
  eye-like-lib
  obj-file-loader-lib
  thread-safe-queue-lib
  ${OPENGL_LIBRARY}
  glfw
  glog
  ${OpenCV_LIBS}
  ${Boost_PROGRAM_OPTIONS_LIBRARY})
create_target_launcher(3d-telecom WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET 3d-telecom
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/3d-telecom${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# ========== video-capture-speed ==========
add_executable(video-capture-speed src/video_capture_speed.cpp)
target_link_libraries(video-capture-speed realsense2 ${OpenCV_LIBS})
create_target_launcher(video-capture-speed WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET video-capture-speed
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/video-capture-speed${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# ========== rgbd-to-3d ==========
add_executable(rgbd-to-3d src/rgbd_to_3d.cpp)
target_link_libraries(rgbd-to-3d eye-like-lib ${OpenCV_LIBS} ${OPENGL_LIBRARY}
                      realsense2 glfw)
create_target_launcher(rgbd-to-3d WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET rgbd-to-3d
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/rgbd-to-3d${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# ========== camera ==========
add_library(camera-lib src/camera.cpp)
add_executable(camera src/camera_main.cpp)
target_link_libraries(
  camera
  camera-lib
  thread-safe-queue-lib
  eye-like-lib
  realsense2
  ${OpenCV_LIBS}
  glog)
create_target_launcher(camera WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET camera
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/camera${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# ========== renderer ==========
add_library(renderer-lib src/renderer.cpp src/camera.cpp)
add_executable(renderer src/renderer_main.cpp)
target_link_libraries(
  renderer
  camera-lib
  renderer-lib
  eye-like-lib
  thread-safe-queue-lib
  realsense2
  ${OpenCV_LIBS}
  glog
  glfw
  GLEW_1130)
create_target_launcher(renderer WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET renderer
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/renderer${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# ========== minago ==========
add_executable(minago src/minago.cpp src/connector.cpp src/compress.cpp)
target_link_libraries(
  minago
  camera-lib
  renderer-lib
  eye-like-lib
  thread-safe-queue-lib
  realsense2
  ${OpenCV_LIBS}
  glfw
  GLEW_1130
  ${ZLIB_LIBRARIES}
  glog)
create_target_launcher(minago WORKING_DIRECTORY
                       "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
  TARGET minago
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/minago${CMAKE_EXECUTABLE_SUFFIX}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/")
