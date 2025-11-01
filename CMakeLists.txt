cmake_minimum_required(VERSION 3.24)
project(model_importer)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall ${CMAKE_CXX_FLAGS}")
include_directories("./include")
find_package(Boost COMPONENTS system filesystem REQUIRED)

add_executable(model_importer main.cpp loader.cpp)
target_link_libraries(model_importer ozz_animation ozz_animation_offline ozz_geometry ozz_base assimp lemon ${Boost_LIBRARIES} boost_system boost_filesystem)