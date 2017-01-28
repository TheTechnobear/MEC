project (mec-push2)

set(PUSH_DRIVER_SOURCES
    push2lib.cpp
  )

add_library(mec-push2 SHARED ${PUSH_DRIVER_SOURCES})

find_library(LIBUSB_LIB NAME libusb)

target_link_libraries(mec-push2 libusb)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(mec-push2 atomic)
endif()
