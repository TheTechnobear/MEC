# mec-bela
bela project interfacing to mecapi

#building and running

##pre-requisites
the mec project

    cd ~
    mkdir dev
    git clone http://github.com/TheTechnobear/MEC.git
    mkdir build
    cd build
    cmake ..
    make
    
#status
work in progress!

- soundplane and eigenharp pico tested, and working 
- working against axoloti
- MIDI is MPE only
- (soundplane has high latency, cpu issue? other?)

to do

- bela native sound
- midi input
- osc output
- when MEC is stable, prebuilt mec binaries, and include file, so can be build without MEC git repo
- improve soundplane latency

#notes


makefile flags for this project

    AT=;LDFLAGS=-L/root/dev/MEC/build/release/lib;LDLIBS=-lmecapi -Wl,-rpath,/root/dev/MEC/build/release/lib
