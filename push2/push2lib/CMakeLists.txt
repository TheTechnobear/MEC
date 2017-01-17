project (push2lib)

set(PUSH_DRIVER_SOURCES
    push2lib.cpp
  )

add_library(push2lib SHARED ${PUSH_DRIVER_SOURCES})

target_link_libraries(push2lib libusb)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(push2lib atomic)
endif()
