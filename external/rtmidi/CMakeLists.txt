cmake_minimum_required(VERSION 2.8.11)
project(rtmidi)


if(APPLE)
    set_source_files_properties(RtMidi.cpp PROPERTIES COMPILE_DEFINITIONS __MACOSX_CORE__)
elseif(UNIX)
    set_source_files_properties(RtMidi.cpp PROPERTIES COMPILE_DEFINITIONS __LINUX_ALSA__)

    find_package (ALSA REQUIRED)
    if (ALSA_FOUND)
      include_directories(${ALSA_INCLUDE_DIRS})
    endif (ALSA_FOUND)
endif(APPLE)

add_library(rtmidi 
    RtMidi.cpp
    RtMidi.h
  )

target_include_directories(rtmidi PUBLIC .)
