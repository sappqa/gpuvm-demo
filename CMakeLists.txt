cmake_minimum_required(VERSION 3.20)
project(gpuvm LANGUAGES CUDA CXX)
add_executable(gpuvm gpuvm.cpp page_allocator.cpp)
target_link_libraries(gpuvm PRIVATE cuda)