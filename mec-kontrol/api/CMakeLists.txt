###############################
# MEC library
project (mec-kontrol-api)

set(KONTROL_API_SRC 
    Entity.cpp
    Rack.cpp
    Module.cpp
    Parameter.cpp 
    ParamValue.cpp 
    ParameterModel.cpp
    OSCReceiver.cpp 
    OSCBroadcaster.cpp )

include_directories (
    "${PROJECT_SOURCE_DIR}/../../mec-utils"
    "${PROJECT_SOURCE_DIR}/../../external/oscpack"
    "${PROJECT_SOURCE_DIR}/../../external/portaudio"
    "${PROJECT_SOURCE_DIR}/../../external/cJson"
    )

add_library(mec-kontrol-api SHARED ${KONTROL_API_SRC})

target_link_libraries (mec-kontrol-api mec-utils oscpack portaudio cjson)

# add_subdirectory(tests)

