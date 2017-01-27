cmake_minimum_required(VERSION 2.8.11)

project(mec)

set(PROJECT_NAME MEC)
set(MEC_VERSION_MAJOR 0)
set(MEC_VERSION_MINOR 1)
set(MEC_VERSION_UPDATE 0)
set(MEC_VERSION "${MEC_VERSION_MAJOR}.${MEC_VERSION_MINOR}.${MEC_VERSION_UPDATE}")

if(APPLE)
    set(EXECUTABLE_NAME MicroExpression)
elseif(UNIX)
    set(EXECUTABLE_NAME microexpression)
endif(APPLE)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(UNIX) 
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11  -mfloat-abi=hard -mfpu=neon -O3 ")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif ()
endif(APPLE) 

set(CMAKE_MACOSX_RPATH 1)


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/release/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/release/bin)

############
add_subdirectory(external/libusb libusb)
add_subdirectory(external/oscpack oscpack)
add_subdirectory(external/portaudio portaudio)
add_subdirectory(external/cJSON cjson)
add_subdirectory(external/rtmidi rtmidi)

add_subdirectory (mec-api) 
add_subdirectory (mec-app)

