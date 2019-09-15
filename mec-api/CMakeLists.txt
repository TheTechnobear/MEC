###############################
# MEC library
project(mec-api)

if (NOT DISABLE_SOUNDPLANELITE)
    set(SOUNDPLANELITE_SRC
            devices/mec_soundplane.cpp
            devices/mec_soundplane.h
            )


    set(SOUNDPLANELITE_LIB splite)

else()
    add_definitions(-DDISABLE_SOUNDPLANELITE=1)
endif()

if (NOT DISABLE_EIGENHARP)
    set(EIGENHARP_SRC
            devices/mec_eigenharp.cpp
            devices/mec_eigenharp.h
            mec_voice.h
            )

    set(EIGENHARP_LIB eigenapi)
else()
    add_definitions(-DDISABLE_EIGENHARP=1)
endif()

if (NOT DISABLE_PUSH2)
    set(PUSH2_SRC
            devices/mec_push2.cpp
            devices/mec_push2.h
            devices/push2/mec_push2_param.cpp
            devices/push2/mec_push2_param.h
            devices/push2/mec_push2_module.cpp
            devices/push2/mec_push2_module.h
            devices/push2/mec_push2_play.h
            devices/push2/mec_push2_play.cpp
            devices/push2/mec_push2_preset.cpp
            devices/push2/mec_push2_preset.h
            )

    add_subdirectory(devices/push2)

    set(PUSH2_LIB mec-push2)

    include_directories("devices/push2")
else()
    add_definitions(-DDISABLE_PUSH2=1)
endif()

if (NOT DISABLE_OSCDISPLAY)
    set(OSCDISPLAY_SRC
            devices/mec_oscdisplay.cpp
            devices/mec_oscdisplay.h
            )

#    add_subdirectory(devices/oscdisplay)
#    set(OSCDISPLAY_LIB mec-oscdisplay)
#    include_directories("devices/oscdisplay")
else()
    add_definitions(-DDISABLE_OSCDISPLAY=1)
endif()

if (NOT DISABLE_FATES)
    set(FATES_SRC
            devices/mec_fates.cpp
            devices/mec_fates.h
            )
    set(FATESLITE_LIB fateslite)
else()
    add_definitions(-DDISABLE_FATES=1)
endif()


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
        ${SOUNDPLANELITE_SRC}
        ${EIGENHARP_SRC}
        ${PUSH2_SRC}
        ${OSCDISPLAY_SRC}
        ${FATES_SRC}
        )

include_directories(
        "devices/push2"
        "${PROJECT_SOURCE_DIR}/../mec-utils"
        "${PROJECT_SOURCE_DIR}/../mec-kontrol/api"
)

add_library(mec-api SHARED ${MECAPI_SRC})

set(MEC_DEVICE_LIBS ${PUSH2_LIB} ${EIGENHARP_LIB} ${SOUNDPLANELITE_LIB} ${FATESLITE_LIB})

target_link_libraries(mec-api mec-utils ${MEC_DEVICE_LIBS}  mec-kontrol-api cjson oscpack rtmidi portaudio moodycamel)
set_target_properties(mec-api PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)
add_subdirectory(tests)

target_include_directories(mec-api PUBLIC .)
