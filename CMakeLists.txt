cmake_minimum_required(VERSION 3.24)
project(mesh_importer)

#set(CMAKE_BUILD_TYPE "DEBUG")
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall ${CMAKE_CXX_FLAGS}")

find_package(Boost COMPONENTS system filesystem REQUIRED)

add_executable(mesh_importer main.cpp loader.cpp schema.capnp.c++)
target_link_libraries(mesh_importer  ozz_animation ozz_animation_offline ozz_geometry ozz_base assimp lemon ${Boost_LIBRARIES})

#project(test_mesh_output)
#add_executable(test_mesh_output test_mesh_output.cpp)
