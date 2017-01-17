cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_MACOSX_RPATH 1)

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

############
add_subdirectory(external/libusb libusb)
add_subdirectory(external/oscpack oscpack)
add_subdirectory(external/portaudio portaudio)
add_subdirectory(external/cJSON cjson)
add_subdirectory(external/rtmidi rtmidi)


add_subdirectory (eigenharp) 
add_subdirectory (soundplanelite) 
add_subdirectory (push2) 
add_subdirectory (mecapi) 
add_subdirectory (mec)
add_subdirectory (mec.old)


