#CMake entry point
cmake_minimum_required(VERSION 3.0)
project(3d-telecom)

find_package(OpenGL REQUIRED) 
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)

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
    external/AntTweakBar-1.16/include/
    external/glfw-3.1.2/include/
    external/glm-0.9.7.1/
    external/glew-1.13.0/include/ 
    external/assimp-3.0.1270/include/
    external/bullet-2.81-rev2613/src/
    .
)

set(ALL_LIBS 
	${OPENGL_LIBRARY}
    ${OpenCV_LIBS}
	glfw 
	GLEW_1130
    realsense2
)

add_definitions(
    -DTW_STATIC 
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)

add_executable(render2
	src/render2.cpp
    src/obj_file_loader.cpp
)

target_link_libraries(render2 ${ALL_LIBS})

 create_target_launcher(
     render2 WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

 add_custom_command(
     TARGET render2 POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/render2${CMAKE_EXECUTABLE_SUFFIX}"
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

# rs-pointcloud
add_executable(rs-pointcloud
    src/rs-pointcloud.cpp
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
    
target_link_libraries(rs-pointcloud ${ALL_LIBS})
create_target_launcher(rs-pointcloud WORKING_DIRECTORY
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")
add_custom_command(
     TARGET rs-pointcloud POST_BUILD
         COMMAND ${CMAKE_COMMAND} -E copy
     "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/rs-pointcloud${CMAKE_EXECUTABLE_SUFFIX}"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/")

