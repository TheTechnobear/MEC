project (push2lib)

set(CMAKE_MACOSX_RPATH 1)

set(PUSH_DRIVER_SOURCES
    push2lib.cpp
  )

set(PUSH_LIBUSB_DIR yes)

if(PUSH_LIBUSB_DIR OR NOT APPLE)
  set(PUSH_USE_LIBUSB yes)
endif()

add_library(push2lib SHARED ${PUSH_DRIVER_SOURCES})

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
