project(mecold)

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
)
include_directories (
    "${PROJECT_SOURCE_DIR}/../mecapi" 
    "${PROJECT_SOURCE_DIR}/../push2" 
    "${PROJECT_SOURCE_DIR}/../soundplanelite"
    "${PROJECT_SOURCE_DIR}/../eigenharp" 
    "${PROJECT_SOURCE_DIR}/../external/oscpack"
    "${PROJECT_SOURCE_DIR}/../external/portaudio"
    "${PROJECT_SOURCE_DIR}/../external/cJSON"
)

add_executable(mecold ${MEC_SRC})
target_link_libraries (mecold eigenharplib soundplanelite push2lib mecapi cjson rtmidi)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(mecold atomic)
endif()

if(APPLE)
    target_link_libraries(mecold  "-framework CoreMIDI")
elseif(UNIX) 
    target_link_libraries(mecold  "libusb" "dl" "pthread" "asound")
    target_link_libraries (mecold ${ALSA_LIBRARIES})
endif(APPLE)

