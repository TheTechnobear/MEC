project(mec)

set(MEC_SRC 
    mec.h
    mec.cpp
    osc_cmd.cpp
    mecapi_cmd.cpp
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

add_executable(mec ${MEC_SRC})

target_link_libraries (mec eigenharplib soundplanelite push2lib mecapi cjson rtmidi)

if(APPLE)
    target_link_libraries(mec "-framework CoreMIDI")
endif(APPLE)

