###############################
# MEC library
project (mec-kontrol-api)

set(KONTROL_API_SRC 
    ParameterModel.cpp
    Parameter.cpp 
    OSCReceiver.cpp 
    OSCBroadcaster.cpp )

include_directories (
    "${PROJECT_SOURCE_DIR}/../../external/oscpack"
    "${PROJECT_SOURCE_DIR}/../../external/portaudio"
    )

add_library(mec-kontrol-api SHARED ${KONTROL_API_SRC})

target_link_libraries (mec-kontrol-api oscpack portaudio)

# add_subdirectory(tests)

