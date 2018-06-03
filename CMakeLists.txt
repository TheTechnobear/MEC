cmake_minimum_required(VERSION 2.8.11)

project(mec)

set(PROJECT_NAME MEC)
set(MEC_VERSION_MAJOR 0)
set(MEC_VERSION_MINOR 1)
set(MEC_VERSION_UPDATE 1)
set(MEC_VERSION "${MEC_VERSION_MAJOR}.${MEC_VERSION_MINOR}.${MEC_VERSION_UPDATE}")
set(CMAKE_OSX_ARCHITECTURES "x86_64")

if(WIN32)
    message("windows support limited")
    set(DISABLE_LIBUSB 1)
    set(DISABLE_EIGENHARP 1)
    set(DISABLE_SOUNDPLANELITE 1)
    set(DISABLE_PUSH2 1)
endif()

if (APPLE)
    set(EXECUTABLE_NAME MicroExpression)
elseif (UNIX)
    set(EXECUTABLE_NAME microexpression)
endif (APPLE)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -fomit-frame-pointer")


message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
if (APPLE)
elseif (UNIX)
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
        if (BELA)
            message(STATUS "BELA optimized")
	    # /usr/xenomai/bin/xeno-config --skin=cobalt --no-mode-check --cflags
	    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/xenomai/include/cobalt -I/usr/xenomai/include -march=armv7-a -mfpu=vfp3 -D_GNU_SOURCE -D_REENTRANT -D__COBALT__")
	    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,@${PROJECT_SOURCE_DIR}/xenomai.wrappers")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mtune=cortex-a8 -mfloat-abi=hard -mfpu=neon -ftree-vectorize --fast-math")
	    #  /usr/xenomai/bin/xeno-config --skin=cobalt --no-auto-init --no-mode-check --ldflags
	    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed -L/usr/xenomai/lib -lcobalt -lmodechk -lpthread -lrt ")
	    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-as-needed -L/usr/xenomai/lib -lcobalt -lmodechk -lpthread -lrt ")
	    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed -L/usr/xenomai/lib -lcobalt -lmodechk -lpthread -lrt ")
	    #set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -Wl,--no-as-needed -L/usr/xenomai/lib -lcobalt -lmodechk -lpthread -lrt ")
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
if(NOT DISABLE_LIBUSB)
add_subdirectory(external/libusb libusb)
endif()

add_subdirectory(external/oscpack oscpack)
add_subdirectory(external/portaudio portaudio)
add_subdirectory(external/cJSON cjson)
add_subdirectory(external/rtmidi rtmidi)
add_subdirectory(external/moodycamel)


add_subdirectory(mec-utils)
add_subdirectory(mec-kontrol)
add_subdirectory(mec-api)
add_subdirectory(mec-app)

