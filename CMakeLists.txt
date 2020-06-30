#CMake entry point
cmake_minimum_required(VERSION 3.0)
project(minago)

find_package(OpenGL REQUIRED) 
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(ZLIB)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_BINARY_DIR ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 17)

#Compile external dependencies
add_subdirectory(external)

#On Visual 2005 and above, this module can set the debug working directory
cmake_policy(SET CMP0026 OLD)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(CreateLaunchers)

include_directories(
    external/glfw-3.1.2/include/
    external/glm-0.9.7.1/
    external/glew-1.13.0/include/ 
    external/librealsense-2.35.2/include/
    external/glog-0.4.0/
    external/glog-0.4.0/src/
    .
)

set(ALL_LIBS 
	${OPENGL_LIBRARY}
    ${OpenCV_LIBS}
    ${ZLIB_LIBRARIES}
	glfw 
	GLEW_1130
    realsense2
    glog
)

add_definitions(
    -DTW_STATIC 
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)

add_executable(obj_file_loader
	src/obj_file_loader_main.cpp
    src/obj_file_loader.cpp
)

target_link_libraries(obj_file_loader ${ALL_LIBS})

 create_target_launcher(
     obj_file_loader WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

 add_custom_command(
     TARGET obj_file_loader POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/obj_file_loader${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

 add_executable(eyeLike
     src/eyeLikeMain.cpp 
     src/eyeLike.cpp 
     src/eyeLike.h
     src/findEyeCenter.cpp 
     src/findEyeCenter.h
     src/findEyeCorner.cpp
     src/findEyeCorner.h 
     src/constants.h
     src/helpers.cpp
     src/helpers.h)
 
 target_link_libraries(eyeLike ${ALL_LIBS})
 
 create_target_launcher(eyeLike WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
 
 add_custom_command(
     TARGET eyeLike POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/eyeLike${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
     
add_executable(3d_telecom
   src/3d_telecom.cpp
   src/obj_file_loader.cpp
   src/eyeLike.cpp 
   src/eyeLike.h
   src/findEyeCenter.cpp 
   src/findEyeCenter.h
   src/findEyeCorner.cpp
   src/findEyeCorner.h 
   src/constants.h
   src/helpers.cpp
   src/helpers.h
   src/thread_safe_queue.cpp
   )

target_link_libraries(3d_telecom ${ALL_LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})

create_target_launcher(3d_telecom WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
 
add_custom_command(
     TARGET 3d_telecom POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/3d_telecom${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# video_capture_speed
add_executable(video_capture_speed
   src/video_capture_speed.cpp)
target_link_libraries(video_capture_speed ${ALL_LIBS})
create_target_launcher(video_capture_speed WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
     TARGET video_capture_speed POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/video_capture_speed${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# rgbd-to-3d
add_executable(rgbd-to-3d
    src/rgbd-to-3d.cpp
    src/example.hpp
    src/eyeLike.cpp 
    src/eyeLike.h
    src/findEyeCenter.cpp 
    src/findEyeCenter.h
    src/findEyeCorner.cpp
    src/findEyeCorner.h 
    src/constants.h
    src/helpers.cpp
    src/helpers.h)
    
target_link_libraries(rgbd-to-3d ${ALL_LIBS})
create_target_launcher(rgbd-to-3d WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
     TARGET rgbd-to-3d POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/rgbd-to-3d${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# camera
add_executable(camera
    src/camera_main.cpp
    src/camera.cpp
    src/example.hpp
    src/eyeLike.cpp 
    src/eyeLike.h
    src/findEyeCenter.cpp 
    src/findEyeCenter.h
    src/findEyeCorner.cpp
    src/findEyeCorner.h 
    src/constants.h
    src/helpers.cpp
    src/helpers.h
    src/thread_safe_queue.cpp
    src/thread_safe_queue.h)
    
target_link_libraries(camera ${ALL_LIBS})
create_target_launcher(camera WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
     TARGET camera POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/camera${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# renderer
add_executable(renderer
    src/renderer_main.cpp
    src/renderer.cpp
    src/camera.cpp
    src/example.hpp
    src/eyeLike.cpp 
    src/findEyeCenter.cpp 
    src/findEyeCorner.cpp
    src/helpers.cpp
    src/thread_safe_queue.cpp
    src/thread_safe_queue.h)
    
target_link_libraries(renderer ${ALL_LIBS})
create_target_launcher(renderer WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
     TARGET renderer POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/renderer${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# point-cloud-compress 
add_executable(point-cloud-compress
    src/point-cloud-compress.cpp
    src/example.hpp
    src/eyeLike.cpp 
    src/eyeLike.h
    src/findEyeCenter.cpp 
    src/findEyeCenter.h
    src/findEyeCorner.cpp
    src/findEyeCorner.h 
    src/constants.h
    src/helpers.cpp
    src/helpers.h
    src/thread_safe_queue.cpp
    src/thread_safe_queue.h)
    
target_link_libraries(point-cloud-compress ${ALL_LIBS})
create_target_launcher(point-cloud-compress WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
     TARGET point-cloud-compress POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/point-cloud-compress${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# minago
add_executable(minago
    src/minago.cpp
    src/camera.cpp
    src/renderer.cpp
    src/example.hpp
    src/eyeLike.cpp 
    src/eyeLike.h
    src/findEyeCenter.cpp 
    src/findEyeCenter.h
    src/findEyeCorner.cpp
    src/findEyeCorner.h 
    src/constants.h
    src/connector.cpp
    src/helpers.cpp
    src/helpers.h
    src/compress.cpp
    src/thread_safe_queue.cpp
    src/thread_safe_queue.h)
    
target_link_libraries(minago ${ALL_LIBS})
create_target_launcher(minago WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
     TARGET minago POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/camera${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

