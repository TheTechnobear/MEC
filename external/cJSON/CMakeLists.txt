cmake_minimum_required(VERSION 2.8.11)
project(cJSON)

set(CMAKE_MACOSX_RPATH 1)

add_library(cjson SHARED
  cJSON.c
  cJSON.h
  )
target_include_directories(cjson PUBLIC .)
