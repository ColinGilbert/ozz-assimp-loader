cmake_minimum_required(VERSION 3.0)

project(mesh_importer)

#set(CMAKE_BUILD_TYPE "DEBUG")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall ${CMAKE_CXX_FLAGS}")

find_package(Boost COMPONENTS system filesystem REQUIRED)

include_directories()
include_directories(libs/lemon)
include_directories(libs/cppformat)
list (APPEND cppformat_src
	libs/cppformat/format.cc
	libs/cppformat/posix.cc
	)


add_executable(mesh_importer main.cpp ${cppformat_src} ${ozz_src})

target_link_libraries(mesh_importer assimp emon ozz_animation_offline_r	ozz_animation_r ozz_geometry_r ozz_base_r ${Boost_LIBRARIES})
