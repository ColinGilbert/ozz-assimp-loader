cmake_minimum_required(VERSION 3.0)

project(mesh_importer)

set(CMAKE_BUILD_TYPE "DEBUG")
set(CMAKE_CXX_FLAGS "-std=c++11 -fpermissive -Wall ${CMAKE_CXX_FLAGS}")

include_directories(libs/lemon)

include_directories(libs/cppformat)
list (APPEND cppformat_src
	libs/cppformat/format.cc
	libs/cppformat/posix.cc
	)

include_directories(libs/ozz-animation/include)

list (APPEND ozz_src
	libs/ozz-animation/src/geometry/runtime/skinning_job.cc
	libs/ozz-animation/src/base/memory/allocator.cc
	libs/ozz-animation/src/base/containers/string_archive.cc
	libs/ozz-animation/src/base/io/archive.cc
	libs/ozz-animation/src/base/io/stream.cc
	libs/ozz-animation/src/base/maths/soa_math_archive.cc
	libs/ozz-animation/src/base/maths/math_archive.cc
	libs/ozz-animation/src/base/maths/simd_math_archive.cc
	libs/ozz-animation/src/base/maths/box.cc
	libs/ozz-animation/src/base/log.cc
	libs/ozz-animation/src/options/options.cc
	libs/ozz-animation/src/animation/offline/raw_skeleton.cc
	libs/ozz-animation/src/animation/offline/skeleton_builder.cc
	libs/ozz-animation/src/animation/offline/raw_animation.cc
	# This file relies on a collada_transform.h file (which does not exist anymore)
	#libs/ozz-animation/src/animation/offline/collada/collada_transform.cc
	#libs/ozz-animation/src/animation/offline/collada/collada.cc
	#libs/ozz-animation/src/animation/offline/collada/collada_animation.cc
	#libs/ozz-animation/src/animation/offline/collada/dae2anim.cc
	#libs/ozz-animation/src/animation/offline/collada/collada_skeleton.cc
	#libs/ozz-animation/src/animation/offline/collada/collada_base.cc
	#libs/ozz-animation/src/animation/offline/collada/dae2skel.cc
	#libs/ozz-animation/src/animation/offline/fbx/fbx2skel.cc
	#libs/ozz-animation/src/animation/offline/fbx/fbx2anim.cc
	#libs/ozz-animation/src/animation/offline/fbx/fbx_base.cc
	#libs/ozz-animation/src/animation/offline/fbx/fbx_animation.cc
	#libs/ozz-animation/src/animation/offline/fbx/fbx.cc
	#libs/ozz-animation/src/animation/offline/fbx/fbx_skeleton.cc
	#libs/ozz-animation/src/animation/offline/animation_builder.cc
	libs/ozz-animation/src/animation/offline/raw_skeleton_archive.cc
	libs/ozz-animation/src/animation/offline/animation_optimizer.cc
	#libs/ozz-animation/src/animation/offline/tools/convert2anim.cc
	#libs/ozz-animation/src/animation/offline/tools/convert2skel.cc
	libs/ozz-animation/src/animation/offline/raw_animation_archive.cc
	libs/ozz-animation/src/animation/runtime/blending_job.cc
	libs/ozz-animation/src/animation/runtime/skeleton.cc
	libs/ozz-animation/src/animation/runtime/local_to_model_job.cc
	libs/ozz-animation/src/animation/runtime/skeleton_utils.cc
	libs/ozz-animation/src/animation/runtime/sampling_job.cc
	libs/ozz-animation/src/animation/runtime/animation.cc
	)

add_executable(mesh_importer mesh_import.cpp ${cppformat_src} ${ozz_src})

target_link_libraries(mesh_importer assimp emon)
