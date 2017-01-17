project(soundplanelite)

include_directories (
    "${PROJECT_SOURCE_DIR}/../external/libusb"
    "${PROJECT_SOURCE_DIR}/../external/portaudio"
    "${PROJECT_SOURCE_DIR}/../external/cJSON"
    "${PROJECT_SOURCE_DIR}/../external/oscpack"
)

set(SPLite_H
    SoundplaneDriver.h
    InertSoundplaneDriver.h
    SoundplaneModelA.h
    TouchTracker.h

    MLDSP.h
    MLSignal.h
    MLDebug.h
    MLDSPUtils.h
    MLSymbol.h
    MLVector.h
    ThreadUtility.h
    Unpacker.h
    madronalib.h
    Filters2D.h
    MLTextStreamListener.h

    MLProperty.h
    MLParameter.h
    MLModel.h
    MLPath.h
    MLRingBuffer.h
    SoundplaneModel.h
    SoundplaneDataListener.h
    Zone.h
    SoundplaneOSCOutput.h
    SoundplaneMECOutput.h
    MLAppState.h
)

set(SPLite_Src
    source/Filters2D.cpp
    source/MLDSP.cpp
    source/MLDSPUtils.cpp
    source/MLSignal.cpp
    source/MLSymbol.cpp
    source/MLVector.cpp
    source/SoundplaneDriver.cpp
    source/SoundplaneModelA.cpp
    source/ThreadUtility.cpp
    source/TouchTracker.cpp
    source/MLDebug.cpp
    source/MLModel.cpp
    source/SoundplaneModel.cpp
    source/MLProperty.cpp
    source/MLParameter.cpp
    source/InertSoundplaneDriver.cpp
    source/MLPath.cpp
    source/MLRingBuffer.cpp
    source/Zone.cpp
    source/SoundplaneOSCOutput.cpp
    source/SoundplaneMECOutput.cpp
    source/MLAppState.cpp
)

if(APPLE)
  list(APPEND SPLite_Src 
    source/MLDebugMac.mm
    source/MacSoundplaneDriver.cpp
    source/MacSoundplaneDriver.h
    )

if(1)
    # NOFPU
    list(APPEND SPLite_H
        source/nofpu/MLVector.h
    ) 
    list(APPEND SPLite_Src 
            source/nofpu/MLVector.cpp
            #source/nofpu/MLSignal.cpp
    )
else()
    # SSE
    add_definitions(-D ML_USE_SSE) 
    list(APPEND SPLite_H
        source/sse/MLVector.h
        source/sse/MLDSP.h
    ) 
    list(APPEND SPLite_Src 
            source/sse/MLVector.cpp
            #source/sse/MLSignal.cpp
    )
endif()

elseif(UNIX) 
  list(APPEND SPLite_Src 
    source/LibusbSoundplaneDriver.cpp
    source/LibusbSoundplaneDriver.h
    )

    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
        # NEON
        add_definitions(-D ML_USE_NEON) 
        list(APPEND SPLite_H
            source/neon/MLVector.h
            source/neon/MLDSP.h
        ) 
        list(APPEND SPLite_Src 
                source/neon/MLVector.cpp
                #source/neon/MLSignal.cpp
        )
    else()
        # SSE
        add_definitions(-D ML_USE_SSE) 
        list(APPEND SPLite_H
            source/sse/MLVector.h
            source/sse/MLDSP.h
        ) 
        list(APPEND SPLite_Src 
                source/sse/MLVector.cpp
                #source/sse/MLSignal.cpp
        )
    endif ()
endif(APPLE) 

add_library(soundplanelite SHARED ${SPLite_Src} ${SPLite_H})

target_link_libraries (soundplanelite oscpack portaudio cjson)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(soundplanelite atomic)
endif()

if(APPLE)
    target_link_libraries(soundplanelite  "-framework Cocoa  -framework IOKit -framework CoreAudio")
elseif(UNIX) 
    target_link_libraries(soundplanelite  "libusb" "dl" "pthread" "asound")
    target_link_libraries(soundplanelite ${ALSA_LIBRARIES})
endif(APPLE)

target_include_directories(soundplanelite PUBLIC .)

add_subdirectory(tests)


