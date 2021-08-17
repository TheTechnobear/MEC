cmake_minimum_required(VERSION 3.0)


include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-api/script/max-pretarget.cmake)


include_directories( 
	"${C74_INCLUDES}"
    "../../../../mec-api"
)

link_directories(../../../../build/release/lib)


set(CMAKE_OSX_ARCHITECTURES "x86_64")


add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_NAME}.cpp
)



SET(CMAKE_SKIP_BUILD_RPATH  TRUE)
# SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/Frameworks")
# SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

target_link_libraries(${PROJECT_NAME} mec-api)

message( STATUS "install ${INSTALL_NAME_DIR} ")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-api/script/max-posttarget.cmake)




# set(CMAKE_INSTALL_RPATH "/Users/kodiak/Projects/MEC/build/release/lib")


