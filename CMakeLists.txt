cmake_minimum_required(VERSION "2.8")

project(p3async_prj)

#include_directories(${p3async_prj_SOURCE_DIR}/src/include)
#link_directories(${p3async_prj_BINARY_DIR}/src)

add_definitions(-std=c++11)
add_subdirectory(${p3async_prj_SOURCE_DIR}/src)
#add_subdirectory(${p3async_prj_SOURCE_DIR}/test)

