project(mec-app)

set(MEC_SRC 
    mec_app.h
    mec_app.cpp
    osc_cmd.cpp
    mecapi_cmd.cpp
    # push2_cmd.cpp
    midi_output.cpp
    midi_output.h
)

# include_directories (
#     "${PROJECT_SOURCE_DIR}/../mecapi" 
#     "${PROJECT_SOURCE_DIR}/../push2" 
#     "${PROJECT_SOURCE_DIR}/../soundplanelite"
#     "${PROJECT_SOURCE_DIR}/../eigenharp" 
#     "${PROJECT_SOURCE_DIR}/../external/oscpack"
#     "${PROJECT_SOURCE_DIR}/../external/portaudio"
#     "${PROJECT_SOURCE_DIR}/../external/cJSON"
# )

include_directories (
    "${PROJECT_SOURCE_DIR}/../mec-api" 
    "${PROJECT_SOURCE_DIR}/../external/oscpack"
    "${PROJECT_SOURCE_DIR}/../external/rtmidi"
)

add_executable(mec-app ${MEC_SRC})

#target_link_libraries (mec eigenharplib soundplanelite push2lib mecapi cjson rtmidi)
target_link_libraries (mec-app mec-api oscpack rtmidi)

if(APPLE)
    target_link_libraries(mec-app "-framework CoreMIDI")
endif(APPLE)

