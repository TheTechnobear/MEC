###############################
# MEC library
project (mec-api)

add_subdirectory (devices/eigenharp) 
add_subdirectory (devices/soundplanelite) 
add_subdirectory (devices/push2) 

set(MECAPI_SRC 
    mec_api.cpp
    mec_api.h
    mec_device.h
    mec_eigenharp.cpp
    mec_eigenharp.h
    mec_log.h
    mec_msg_queue.cpp
    mec_msg_queue.h
    mec_prefs.cpp
    mec_prefs.h
    mec_soundplane.cpp
    mec_soundplane.h
    mec_surfacemapper.cpp
    mec_surfacemapper.h
    mec_voice.h
)

include_directories (
    "devices/push2" 
    "devices/eigenharp" 
    "devices/soundplanelite"
    "${PROJECT_SOURCE_DIR}/../external/cJSON"
    "${PROJECT_SOURCE_DIR}/../external/oscpack"
    )

add_library(mec-api SHARED ${MECAPI_SRC})

target_link_libraries (mec-api mec-eigenharp mec-soundplane mec-push2 cjson oscpack)
