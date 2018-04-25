###############################
# MEC library
project(mec-api)

if (NOT WIN32)
    set(MECDEVICES_SRC
            devices/mec_eigenharp.cpp
            devices/mec_eigenharp.h
            devices/mec_push2.cpp
            devices/mec_push2.h
            devices/push2/mec_push2_param.cpp
            devices/push2/mec_push2_param.h
            devices/push2/mec_push2_device.cpp
            devices/push2/mec_push2_device.h
            devices/push2/mec_push2_play.h
            devices/push2/mec_push2_play.cpp
            devices/mec_soundplane.cpp
            devices/mec_soundplane.h
            )

    add_subdirectory(devices/eigenharp)
    add_subdirectory(devices/soundplanelite)
    add_subdirectory(devices/push2)
endif ()

set(MECAPI_SRC
        mec_api.cpp
        mec_api.h
        mec_device.h
        mec_msg_queue.cpp
        mec_msg_queue.h
        mec_scaler.cpp
        mec_scaler.h
        mec_surface.cpp
        mec_surface.h
        mec_surfacemapper.cpp
        mec_surfacemapper.h
        mec_voice.h
        processors/mec_midi_processor.cpp
        processors/mec_midi_processor.h
        processors/mec_mpe_processor.cpp
        processors/mec_mpe_processor.h
        devices/mec_mididevice.cpp
        devices/mec_mididevice.h
        devices/mec_osct3d.cpp
        devices/mec_osct3d.h
        devices/mec_kontroldevice.cpp
        devices/mec_kontroldevice.h
        ${MECDEVICES_SRC}
        )

include_directories(
        "devices/push2"
        "devices/eigenharp"
        "devices/soundplanelite"
        "${PROJECT_SOURCE_DIR}/../mec-kontrol/api"
)

add_library(mec-api SHARED ${MECAPI_SRC})

if(NOT WIN32)
    set(MEC_DEVICE_LIBS mec-eigenharp mec-soundplane mec-push2)
endif()

target_link_libraries(mec-api mec-utils ${MEC_DEVICE_LIBS}  mec-kontrol-api cjson oscpack rtmidi)
set_target_properties(mec-api PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)
add_subdirectory(tests)

target_include_directories(mec-api PUBLIC .)
