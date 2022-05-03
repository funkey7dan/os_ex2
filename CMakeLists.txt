cmake_minimum_required(VERSION 3.21)
project(submit C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(submit
        ex21.c ex22.c)
