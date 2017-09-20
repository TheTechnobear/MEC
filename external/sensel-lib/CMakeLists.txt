project (sensel)

# decompress library not covered by MIT license
#add_definitions(-DSENSEL_PRESSURE)
#set(CMAKE_SHARED_LINKER_FLAGS -lsenseldecompress)

set(SENSEL_LIB_SRC
        src/sensel.c
        src/sensel_register.c
        )

if(APPLE)
    set(SENSEL_LIB_SRC ${SENSEL_LIB_SRC}
            src/sensel_serial_linux.c)
elseif(UNIX)
    set(SENSEL_LIB_SRC ${SENSEL_LIB_SRC}
            src/sensel_serial_linux.c)
elseif(WIN32)
    set(SENSEL_LIB_SRC ${SENSEL_LIB_SRC}
            src/sensel_serial_win.c)
endif(APPLE)

include_directories (src)

add_library(sensel SHARED ${SENSEL_LIB_SRC})

target_link_libraries(sensel)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(sensel atomic)
endif()

target_include_directories(sensel PUBLIC .)