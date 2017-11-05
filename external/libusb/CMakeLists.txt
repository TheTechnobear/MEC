if (SP_LIBUSB_DIR)
    # For debugging purposes, it's possible to include the source of libusb
    # directly into the project.
    add_library(libusb
            "${SP_LIBUSB_DIR}/libusb/core.c"
            "${SP_LIBUSB_DIR}/libusb/descriptor.c"
            "${SP_LIBUSB_DIR}/libusb/hotplug.c"
            "${SP_LIBUSB_DIR}/libusb/io.c"
            "${SP_LIBUSB_DIR}/libusb/sync.c"
            "${SP_LIBUSB_DIR}/libusb/core.c"
            "${SP_LIBUSB_DIR}/libusb/os/darwin_usb.c"
            "${SP_LIBUSB_DIR}/libusb/os/poll_posix.c"
            "${SP_LIBUSB_DIR}/libusb/os/threads_posix.c")
    target_include_directories(libusb SYSTEM PUBLIC "${SP_LIBUSB_DIR}/libusb")
    target_include_directories(libusb SYSTEM PRIVATE "${SP_LIBUSB_DIR}")
    add_definitions(
            -DLIBUSB_DESCRIBE="")
else (SP_LIBUSB_DIR)
    include(findlibusb.cmake)

    if (NOT LIBUSB_1_FOUND)
        message(FATAL_ERROR "Did not find libusb")
    endif ()

    add_library(libusb empty.c)
    target_link_libraries(libusb ${LIBUSB_1_LIBRARIES})
    target_include_directories(libusb PUBLIC ${LIBUSB_1_INCLUDE_DIRS})
endif ()

if (APPLE)
    target_link_libraries(libusb "-framework IOKit")
endif ()

