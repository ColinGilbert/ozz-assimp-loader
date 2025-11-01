cmake_minimum_required(VERSION 3.24)
project(mesh_importer)

#set(CMAKE_BUILD_TYPE "DEBUG")
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall ${CMAKE_CXX_FLAGS}")
include_directories("./include")
find_package(Boost COMPONENTS system filesystem REQUIRED)
#find_package(msgpack-cxx REQUIRED)

add_executable(mesh_importer main.cpp loader.cpp)
target_link_libraries(mesh_importer ozz_animation ozz_animation_offline ozz_geometry ozz_base assimp lemon ${Boost_LIBRARIES} boost_system boost_filesystem)