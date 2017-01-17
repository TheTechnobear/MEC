project(cJSON)

add_library(cjson SHARED
  cJSON.c
  cJSON.h
  )
target_include_directories(cjson PUBLIC .)
