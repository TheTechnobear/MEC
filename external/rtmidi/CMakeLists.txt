project(rtmidi)


if (APPLE)
    set_source_files_properties(RtMidi.cpp PROPERTIES COMPILE_DEFINITIONS __MACOSX_CORE__)
elseif (UNIX)
    set_source_files_properties(RtMidi.cpp PROPERTIES COMPILE_DEFINITIONS __LINUX_ALSA__)

    find_package(ALSA REQUIRED)
    if (ALSA_FOUND)
        include_directories(${ALSA_INCLUDE_DIRS})
    endif (ALSA_FOUND)
endif (APPLE)

add_library(rtmidi SHARED
        RtMidi.cpp
        RtMidi.h
        )


if (APPLE)
    target_link_libraries(rtmidi "-framework Cocoa -framework CoreMIDI -framework CoreAudio")
elseif (UNIX)
    target_link_libraries(rtmidi ${ALSA_LIBRARIES})
endif (APPLE)

set_target_properties(rtmidi PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)

target_include_directories(rtmidi PUBLIC .)
