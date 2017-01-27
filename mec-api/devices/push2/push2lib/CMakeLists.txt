project (mec-push2)

set(PUSH_DRIVER_SOURCES
    push2lib.cpp
  )

add_library(mec-push2 SHARED ${PUSH_DRIVER_SOURCES})

target_link_libraries(mec-push2 libusb)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(mec-push2 atomic)
endif()
