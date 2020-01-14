cmake_minimum_required(VERSION 2.8)
project(StepikWebServer)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall")
add_executable(final main.cpp)
target_link_libraries(final uv)