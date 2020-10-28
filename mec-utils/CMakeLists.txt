###############################
# MEC library
project(mec-utils)

set(MECUTILS_SRC
        mec_log.h
        mec_prefs.cpp
        mec_prefs.h
        mec_utils.cpp mec_utils.h)


add_library(mec-utils SHARED ${MECUTILS_SRC})
set_target_properties(mec-utils PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)
target_link_libraries(mec-utils cjson)

target_include_directories(mec-utils PUBLIC .)
