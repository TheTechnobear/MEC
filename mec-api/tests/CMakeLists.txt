include_directories (
    "${PROJECT_SOURCE_DIR}/../mec-api" 
)

add_executable(t_scale t_scale.cpp)
target_link_libraries (t_scale mec-api )

add_executable(t_voice t_voice.cpp)
target_link_libraries (t_voice mec-api )

add_executable(t_surface t_surface.cpp)
target_link_libraries (t_surface mec-api )

# add_executable(t_kontrol t_kontrol.cpp)
# target_link_libraries (t_surface mec-api )
