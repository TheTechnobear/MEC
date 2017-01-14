project (eigenharplib)

set(CMAKE_MACOSX_RPATH 1)

if(APPLE)
    #add_definitions(-D ML_USE_SSE)
    add_definitions(-D __MACOSX_CORE__)
elseif(UNIX)
    add_definitions(-D __LINUX_ALSA__)

    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
        add_definitions(-D ML_USE_NEON)
    else()
        add_definitions(-D ML_USE_SSE)
    endif ()

    find_package (ALSA REQUIRED)
    if (ALSA_FOUND)
      include_directories(${ALSA_INCLUDE_DIRS})
    endif (ALSA_FOUND)

endif(APPLE)

file (GLOB_RECURSE EIGENHARP_HDRS "*/*.h" )

set(EIGENHARP_SRC
    picross/src/pic_backtrace.c
    picross/src/pic_time.c 
    picross/src/pic_usb_generic.cpp 
    picross/src/usb_serial.cpp 
    picross/src/pic_safeq.cpp
    picross/src/pic_error.cpp 
    picross/src/pic_log.cpp 
    picross/src/pic_fastalloc.cpp 
    picross/src/pic_mlock.cpp 
    picross/src/pic_resources.cpp
    lib_alpha2/src/alpha2_passive.cpp 
    lib_alpha2/src/alpha2_active.cpp
    lib_pico/src/pico_passive.cpp 
    lib_pico/src/pico_active.cpp
    eigenfreed/src/eigenfreed.cpp
    eigenfreed/src/ef_harp.cpp
    eigenfreed/src/ef_alpha.cpp
    eigenfreed/src/ef_pico.cpp
    eigenfreed/src/eigenapi.cpp
)
if(APPLE) 
set(EIGENHARP_SRC ${EIGENHARP_SRC} 
    picross/src/pic_thread_posix.cpp 
    picross/src/pic_cfrunloop.cpp 
    picross/src/pic_usb_macosx.cpp 
    picross/src/pic_tool_macosx.cpp
    picross/src/pic_power_macosx.mm)
elseif(UNIX) 
    set(EIGENHARP_SRC ${EIGENHARP_SRC} 
        picross/src/pic_thread_posix.cpp 
        picross/src/pic_usb_linux.cpp 
        picross/src/pic_tool_linux.cpp)
elseif(WIN32) 
    set(EIGENHARP_SRC ${EIGENHARP_SRC} 
    picross/src/pic_thread_win32.cpp
    picross/src/pic_usb_win32.cpp 
    picross/src/pic_winloop.cpp 
    picross/src/pic_tool_win32.cpp
    )
endif(APPLE)


include_directories ( . ./picross ./eigenfreed ./eigenfreed/src ./lib_alpha2 ./lib_pico ./lib_picodecoder )

add_library(eigenharplib SHARED ${EIGENHARP_SRC} ${EIGENHARP_HDRS} )

target_link_libraries (eigenharplib picodecoder)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(eigenharplib atomic)
endif()

if(APPLE)
    target_link_libraries(eigenharplib  "-framework Cocoa  -framework IOKit -framework CoreAudio")
elseif(UNIX) 
    target_link_libraries(eigenharplib  "libusb" "dl" "pthread" "asound")
    target_link_libraries(eigenharplib ${ALSA_LIBRARIES})
endif(APPLE)


add_subdirectory(tests)
add_subdirectory(lib_picodecoder)

