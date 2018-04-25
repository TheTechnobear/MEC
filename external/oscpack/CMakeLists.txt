project(oscpack)

# separate versions of NetworkingUtils.cpp and UdpSocket.cpp are provided for Win32 and POSIX
# the IpSystemTypePath selects the correct ones based on the current platform

if (WIN32)
    set(IpSystemTypePath ip/win32)
    set(OSCPACKLIBS ${LIBS} Ws2_32 winmm)
else (WIN32)
    set(IpSystemTypePath ip/posix)
endif (WIN32)

add_library(oscpack SHARED

        ip/IpEndpointName.h
        ip/IpEndpointName.cpp

        ip/NetworkingUtils.h
        ${IpSystemTypePath}/NetworkingUtils.cpp

        ip/UdpSocket.h
        ${IpSystemTypePath}/UdpSocket.cpp

        ip/PacketListener.h
        ip/TimerListener.h

        osc/OscTypes.h
        osc/OscTypes.cpp
        osc/OscHostEndianness.h
        osc/OscException.h
        osc/OscPacketListener.h
        osc/MessageMappingOscPacketListener.h
        osc/OscReceivedElements.h
        osc/OscReceivedElements.cpp
        osc/OscPrintReceivedElements.h
        osc/OscPrintReceivedElements.cpp
        osc/OscOutboundPacketStream.h
        osc/OscOutboundPacketStream.cpp

        )

#target_include_directories(oscpack PUBLIC osc)
#target_include_directories(oscpack PUBLIC ip)
#target_include_directories(oscpack PUBLIC zeroconf)
target_include_directories(oscpack PUBLIC .)

target_link_libraries(oscpack ${OSCPACKLIBS})
set_target_properties(oscpack PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS true)

if (MSVC)
    # Force to always compile with W4
    if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif ()
elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # Update if necessary
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
endif ()
