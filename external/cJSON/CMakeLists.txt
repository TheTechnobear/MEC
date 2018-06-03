project(cJSON)

add_library(cjson SHARED
        cJSON.c
        cJSON.h
        )
target_include_directories(cjson PUBLIC .)
set_target_properties(cjson PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)

