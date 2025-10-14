cmake_minimum_required(VERSION 3.10)
project(mesh_importer)

#set(CMAKE_BUILD_TYPE "DEBUG")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall ${CMAKE_CXX_FLAGS}")

find_package(Boost COMPONENTS system filesystem REQUIRED)

add_executable(mesh_importer main.cpp loader.cpp)
target_link_libraries(mesh_importer  ozz_animation_r ozz_animation_offline_r ozz_geometry_r ozz_base_r assimp emon ${Boost_LIBRARIES})

project(test_mesh_output)
include_directories(libs/cereal/include)
add_executable(test_mesh_output test_mesh_output.cpp)
