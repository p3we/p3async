cmake_minimum_required(VERSION "2.8")

project(p3async)

set(
p3async_HEADERS
${p3async_SOURCE_DIR}/include/delegate.hpp
${p3async_SOURCE_DIR}/include/signals.hpp
${p3async_SOURCE_DIR}/include/event_loop.hpp
)

set(
p3async_SOURCES
${p3async_SOURCE_DIR}/event_loop.cpp
)

add_library(p3async SHARED ${p3async_HEADERS} ${p3async_SOURCES})
target_include_directories(p3async PRIVATE ${p3async_SOURCE_DIR}/include)
target_link_libraries(p3async ev)

