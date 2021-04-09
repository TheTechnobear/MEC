# later can go into subdirectory for tests
set(PUSH2TEST_SRC "push2test.cpp")
include_directories ("${PROJECT_SOURCE_DIR}/push2/push2lib")
add_executable(push2test ${PUSH2TEST_SRC})
target_link_libraries (push2test push2lib)
target_link_libraries(push2test "libusb")

set(PUSH2SIMPLE "p2simple.cpp")
add_executable(p2simple ${PUSH2SIMPLE})
target_link_libraries(p2simple "libusb")
