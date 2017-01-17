project(portaudio)

add_library(portaudio SHARED 
  pa_memorybarrier.h
  pa_ringbuffer.cpp
  pa_ringbuffer.h
  )
target_include_directories(portaudio PUBLIC .)
