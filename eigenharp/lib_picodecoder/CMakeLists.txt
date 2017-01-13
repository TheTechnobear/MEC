cmake_minimum_required(VERSION 2.8.11)

project (picodecoder)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_MACOSX_RPATH 1)

include_directories(.)
add_library(picodecoder SHARED pico_decoder.c)
