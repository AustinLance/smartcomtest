cmake_minimum_required(VERSION 3.5.0)
project(mybash VERSION 0.1.0 LANGUAGES C)

add_library(cmd cmd.c)
add_compile_definitions(-D_GNU_SOURCE)

include(CTest)
enable_testing()

