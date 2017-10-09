###############################
# MEC library
project (mec-utils)

set(MECUTILS_SRC 
    mec_log.h
    mec_prefs.cpp
    mec_prefs.h
)

include_directories (
    "${PROJECT_SOURCE_DIR}/../external/cJSON"
)

add_library(mec-utils SHARED ${MECUTILS_SRC})

target_link_libraries (mec-utils cjson)


