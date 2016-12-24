###############################
# MEC app

if(APPLE)
    #add_definitions(-D ML_USE_SSE)
    add_definitions(-D __MACOSX_CORE__)
elseif(UNIX)
    add_definitions(-D __LINUX_ALSA__)

    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
        add_definitions(-D ML_USE_NEON)
    else()
        add_definitions(-D ML_USE_SSE)
    endif ()

    find_package (ALSA REQUIRED)
    if (ALSA_FOUND)
      include_directories(${ALSA_INCLUDE_DIRS})
    endif (ALSA_FOUND)

endif(APPLE)


set(MEC_SRC 
    mec.h
    mec.cpp
    osc_cmd.cpp
    eigen_cmd.cpp
    soundplane_cmd.cpp
    push2_cmd.cpp
    midi_cmd.cpp
    midi_output.cpp
    midi_output.h
    mec_prefs.cpp
    mec_prefs.h
    surfacemapper.cpp
)
include_directories (
    "${PROJECT_SOURCE_DIR}/push2" 
    "${PROJECT_SOURCE_DIR}/eigenharp" 
    "${PROJECT_SOURCE_DIR}/external/oscpack"
    "${PROJECT_SOURCE_DIR}/external/portaudio"
    "${PROJECT_SOURCE_DIR}/external/cJSON"
    "${PROJECT_SOURCE_DIR}/soundplanelite"
    # "${PROJECT_SOURCE_DIR}/external/rtmidi"
#    "${PROJECT_SOURCE_DIR}/soundplane/SoundplaneLib"
#    "${PROJECT_SOURCE_DIR}/soundplane/madronalib"
#    "${PROJECT_SOURCE_DIR}/soundplane/touchtrackerlib"
    )

add_executable(mec ${MEC_SRC})

target_link_libraries (mec eigenharplib soundplanelite push2lib oscpack portaudio cjson rtmidi)
#target_link_libraries (mec eigenharplib soundplanelib touchtrackerlib madronalib oscpack)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
#  target_link_libraries(soundplanelib  "${PROJECT_SOURCE_DIR}/external/libatomic_gcc7/libatomic.a")
  target_link_libraries(mec atomic)
endif()

if(APPLE)
    target_link_libraries(mec  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio -framework CoreMIDI")
elseif(UNIX) 
    target_link_libraries(mec  "libusb" "dl" "pthread" "asound")
    target_link_libraries (mec ${ALSA_LIBRARIES})
endif(APPLE)
