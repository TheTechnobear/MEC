###############################
# MEC library
cmake_minimum_required(VERSION 2.8.11)

project (mecapi)

set(MECAPI_SRC 
    mec_api.cpp
    mec_api.h
    mec_prefs.cpp
    mec_prefs.h
    mec_surfacemapper.cpp
    mec_device.h
    mec_eigenharp.cpp
    mec_eigenharp.h
    mec_soundplane.cpp
    mec_soundplane.h
)

include_directories (
    "${PROJECT_SOURCE_DIR}/../push2" 
    "${PROJECT_SOURCE_DIR}/../eigenharp" 
    "${PROJECT_SOURCE_DIR}/../soundplanelite"
    "${PROJECT_SOURCE_DIR}/../external/cJSON"
    "${PROJECT_SOURCE_DIR}/../external/oscpack"
    "${PROJECT_SOURCE_DIR}/../external/portaudio"
    )

add_library(mecapi SHARED ${MECAPI_SRC})

target_link_libraries (mecapi eigenharplib soundplanelite push2lib cjson)
