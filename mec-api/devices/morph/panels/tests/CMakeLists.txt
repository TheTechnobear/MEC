include_directories (
    #"${PROJECT_SOURCE_DIR}/../../../../../external/sensel-lib"
)

add_executable(t_read_contacts t_read_contacts.cpp)
target_link_libraries (t_read_contacts mec-api)