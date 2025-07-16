cmake_minimum_required(VERSION 3.31)
project(tinygltf)

add_library(tinygltf)
target_sources(tinygltf PRIVATE "src/tiny_gltf.cpp")
target_include_directories(tinygltf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
