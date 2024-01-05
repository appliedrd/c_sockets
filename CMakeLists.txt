cmake_minimum_required(VERSION 3.27)
project(internet_time)

set(CMAKE_CXX_STANDARD 17)

add_executable(internet_time main.cpp
        time_server.c
        time_server.h)

#target_link_libraries(internet_time PRIVATE ws2_32)