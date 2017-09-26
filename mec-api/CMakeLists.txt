###############################
# MEC library
project (mec-api)

add_subdirectory (devices/eigenharp) 
add_subdirectory (devices/soundplanelite) 
add_subdirectory (devices/push2)

set(MECAPI_SRC 
    mec_api.cpp
    mec_api.h
    mec_device.h
    mec_log.h
    mec_msg_queue.cpp
    mec_msg_queue.h
    mec_prefs.cpp
    mec_prefs.h
    mec_scaler.cpp
    mec_scaler.h
    mec_surface.cpp
    mec_surface.h
    mec_surfacemapper.cpp
    mec_surfacemapper.h
    mec_voice.h
    devices/mec_eigenharp.cpp
    devices/mec_eigenharp.h
    devices/mec_mididevice.cpp
    devices/mec_mididevice.h
    devices/mec_osct3d.cpp
    devices/mec_osct3d.h
    devices/mec_push2.cpp
    devices/mec_push2.h
    devices/mec_soundplane.cpp
    devices/mec_soundplane.h
    processors/mec_mpe_processor.cpp
    processors/mec_mpe_processor.h

    #Morph
    devices/mec_morph.cpp
    devices/mec_morph.h
    devices/morph/touches/TouchWithDeltas.h
    devices/morph/touches/Touches.cpp
    devices/morph/touches/Touches.h
    devices/morph/panels/Panel.h
    devices/morph/panels/SinglePanel.cpp
    devices/morph/panels/SinglePanel.h
    devices/morph/panels/CompositePanel.cpp
    devices/morph/panels/CompositePanel.h
    devices/morph/overlays/OverlayFunction.h
    devices/morph/overlays/XYZPlaneOverlay.cpp
    devices/morph/overlays/XYZPlaneOverlay.h
    devices/morph/morph_constants.h
    devices/morph/overlays/OverlayFactory.h
    devices/morph/overlays/OverlayFactory.cpp
    devices/morph/Instrument.cpp
    devices/morph/Instrument.h
)

include_directories (
    "devices/push2" 
    "devices/eigenharp" 
    "devices/soundplanelite"
    "devices/morph"
    "${PROJECT_SOURCE_DIR}/../external/cJSON"
    "${PROJECT_SOURCE_DIR}/../external/oscpack"
    "${PROJECT_SOURCE_DIR}/../external/rtmidi"
    "${PROJECT_SOURCE_DIR}/../external/sensel-lib"
)

add_library(mec-api SHARED ${MECAPI_SRC})

target_link_libraries (mec-api mec-eigenharp mec-soundplane mec-push2 cjson oscpack rtmidi sensel)

add_subdirectory(tests)
add_subdirectory(devices/morph/panels/tests)

