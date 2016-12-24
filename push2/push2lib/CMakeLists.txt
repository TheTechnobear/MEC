set(PUSH_DRIVER_SOURCES
    push2lib.cpp
  )

set(PUSH_LIBUSB_DIR yes)

if(PUSH_LIBUSB_DIR OR NOT APPLE)
  set(PUSH_USE_LIBUSB yes)
endif()

if(PUSH_USE_LIBUSB)
#  set(PUSH_DRIVER_SOURCES
#    ${PUSH_DRIVER_SOURCES}
#    LibusbPush2Driver.cpp
#    LibusbPush2Driver.h
#    )
else(PUSH_USE_LIBUSB)
#  set(PUSH_DRIVER_SOURCES
#    ${PUSH_DRIVER_SOURCES}
#    MacPush2Driver.cpp
#    MacPush2Driver.h
#    )
endif()

add_library(push2lib ${PUSH_DRIVER_SOURCES})

if(PUSH_USE_LIBUSB)
#  add_subdirectory(../../external/libusb libusb)
  target_link_libraries(push2lib libusb)
else()
  target_link_libraries(push2lib "-framework IOKit")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
#  target_link_libraries(push2lib  "${PROJECT_SOURCE_DIR}/external/libatomic_gcc7/libatomic.a")
  target_link_libraries(push2lib atomic)
endif()
