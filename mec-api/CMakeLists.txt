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
    mec_log.h
    mec_msg_queue.cpp
    mec_msg_queue.h
    mec_prefs.cpp
    mec_prefs.h
    mec_surfacemapper.cpp
    mec_surfacemapper.h
    mec_voice.h
    devices/mec_eigenharp.cpp
    devices/mec_eigenharp.h
    devices/mec_mididevice.cpp
    devices/mec_mididevice.h
    devices/mec_osct3d.cpp
    devices/mec_osct3d.h
    devices/mec_push2.cpp
    devices/mec_push2.h
    devices/mec_soundplane.cpp
    devices/mec_soundplane.h
)

include_directories (
    "devices/push2" 
    "devices/eigenharp" 
    "devices/soundplanelite"
    "${PROJECT_SOURCE_DIR}/../external/cJSON"
    "${PROJECT_SOURCE_DIR}/../external/oscpack"
    "${PROJECT_SOURCE_DIR}/../external/rtmidi"
    )

add_library(mec-api SHARED ${MECAPI_SRC})

target_link_libraries (mec-api mec-eigenharp mec-soundplane mec-push2 cjson oscpack rtmidi)
