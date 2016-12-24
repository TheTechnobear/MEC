cmake_minimum_required(VERSION 2.8.11)
project(cJSON)

add_library(cjson
  cJSON.c
  cJSON.h
  )
target_include_directories(cjson PUBLIC .)
