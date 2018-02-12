cmake_minimum_required(VERSION 2.8.11)

project(mec)

set(PROJECT_NAME MEC)
set(MEC_VERSION_MAJOR 0)
set(MEC_VERSION_MINOR 1)
set(MEC_VERSION_UPDATE 0)
set(MEC_VERSION "${MEC_VERSION_MAJOR}.${MEC_VERSION_MINOR}.${MEC_VERSION_UPDATE}")
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

if(WIN32)
    message("windows support limited")
endif()

if (APPLE)
    set(EXECUTABLE_NAME MicroExpression)
elseif (UNIX)
    set(EXECUTABLE_NAME microexpression)
endif (APPLE)

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -fomit-frame-pointer")


message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
if (APPLE)
    #    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif (UNIX)
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
        if (BELA)
            message(STATUS "BELA optimized")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mtune=cortex-a8 -mfloat-abi=hard -mfpu=neon -ftree-vectorize --fast-math")
        else ()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=hard")
        endif (BELA)
    else ()
        #        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall")
    endif ()
endif (APPLE)


## setup rpath for linux and macos
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
if (NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/:$ORIGIN/../lib")
else()
  set(CMAKE_INSTALL_NAME_DIR "@rpath")
  set(CMAKE_INSTALL_RPATH "@executable_path/../lib;@loader_path")
  set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR TRUE)
endif()


set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/release/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/release/bin)

############
if(NOT WIN32)
add_subdirectory(external/libusb libusb)
endif()

add_subdirectory(external/oscpack oscpack)
add_subdirectory(external/portaudio portaudio)
add_subdirectory(external/cJSON cjson)
add_subdirectory(external/rtmidi rtmidi)

add_subdirectory(mec-utils)
add_subdirectory(mec-api)
add_subdirectory(mec-kontrol)
add_subdirectory(mec-app)

