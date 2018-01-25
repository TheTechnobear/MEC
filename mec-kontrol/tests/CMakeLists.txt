include_directories (
    "${PROJECT_SOURCE_DIR}/../mec-kontrol/api" 
    "${PROJECT_SOURCE_DIR}/../mec-utils" 
)

add_executable(t_kontrol t_kontrol.cpp)

target_link_libraries (t_kontrol  mec-kontrol-api mec-utils oscpack portaudio)
if(UNIX)
    target_link_libraries(t_kontrol "pthread")
endif(UNIX)


