###############################
# MEC library
cmake_minimum_required(VERSION 2.8.11)

project (mecapi)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_MACOSX_RPATH 1)

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


set(MECAPI_SRC 
    mec_api.cpp
    mec_api.h
    mec_prefs.cpp
    mec_prefs.h
    mec_surfacemapper.cpp
    mec_device.h
    mec_eigenharp.cpp
    mec_eigenharp.h
)

include_directories (
    "${PROJECT_SOURCE_DIR}/../push2" 
    "${PROJECT_SOURCE_DIR}/../eigenharp" 
    "${PROJECT_SOURCE_DIR}/../soundplanelite"
    "${PROJECT_SOURCE_DIR}/../external/oscpack"
    "${PROJECT_SOURCE_DIR}/../external/portaudio"
    "${PROJECT_SOURCE_DIR}/../external/cJSON"
    # "${PROJECT_SOURCE_DIR}/../external/rtmidi"
    # "${PROJECT_SOURCE_DIR}/../soundplane/SoundplaneLib"
    # "${PROJECT_SOURCE_DIR}/../soundplane/madronalib"
    # "${PROJECT_SOURCE_DIR}/../soundplane/touchtrackerlib"
    )

add_library(mecapi SHARED ${MECAPI_SRC})

target_link_libraries (mecapi eigenharplib picodecoder soundplanelite push2lib oscpack portaudio cjson rtmidi)
#target_link_libraries (mec eigenharplib soundplanelib touchtrackerlib madronalib oscpack)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(mecapi atomic)
endif()

if(APPLE)
    target_link_libraries(mecapi  "-framework CoreServices -framework CoreFoundation -framework IOKit -framework CoreAudio -framework CoreMIDI")
elseif(UNIX) 
    target_link_libraries(mecapi  "libusb" "dl" "pthread" "asound")
    target_link_libraries(mecapi ${ALSA_LIBRARIES})
endif(APPLE)

