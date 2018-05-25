###############################
# kontrol pure data external
project(KontrolRack)


set(KONTROL_HOST_SRC
        KontrolRack.cpp
        KontrolRack.h
        KontrolMonitor.cpp
        KontrolMonitor.h
        dirent_win.cpp
        dirent_win.h
        devices/KontrolDevice.cpp
        devices/KontrolDevice.h
        devices/Organelle.cpp
        devices/Organelle.h
        devices/MidiControl.cpp
        devices/MidiControl.h
        devices/Bela.cpp
        devices/Bela.h
        )

if (${WIN32})
    list(APPEND KONTROL_HOST_SRC KontrolWin.cpp)
else ()
    list(APPEND KONTROL_HOST_SRC KontrolPosix.cpp)
endif()

include_directories(
        "${PROJECT_SOURCE_DIR}/.."
        "${PROJECT_SOURCE_DIR}/../../api"
)

add_library(KontrolRack SHARED ${KONTROL_HOST_SRC})

target_link_libraries(KontrolRack mec-kontrol-api oscpack)

### setup for pure data

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "KontrolRack")


set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
if (${APPLE})
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".pd_darwin")
elseif (${UNIX})
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".pd_linux")
elseif (${WIN32})
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".dll")
    find_library(PD_LIBRARY NAMES pd HINTS ${PD_CMAKE_PATH})
    target_link_libraries(${PROJECT_NAME} ${PD_LIBRARY})
endif ()

# Removes some warning for Microsoft Visual C.
if (${WIN32})
    target_compile_definitions(${PROJECT_NAME} PRIVATE PD_INTERNAL)
    if (${MSVC})
        set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS "/D_CRT_SECURE_NO_WARNINGS /wd4091 /wd4996")
    endif ()
endif ()

