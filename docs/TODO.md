#general
- document configuration file/setup
- improve existing docs

#build issues
- properly package some of the mec apps (some need manual intervention)
- mec-vst need to dylibs into Frameworks
- mec-max need to sort out rpath for external, and package dylibs
- consider a top level scrope to build everything (perhaps options depending on platform being run on... e.g. no point in bela on windows)

#refactor/generalising
- voices data, a few places where I do similar things to collect the data
- midi generation, create a processor
- osc generation, create a processor
- reconsider rtmidi vs juce , rtmidi is dependent on pthread, so perhasp juce

#mec-api , known issues
- voice allocation? (eigenharp looks wrong, in VST)
- push2 , I call render in process(), it probably should be in a separate low prio thread.
- consider libusb initialisation, may be multile libusb devices running

#mec-api , planned changes - mec-api
- osc/t3d input support
- add generic midi/mpe output support to mec-api (MecCallback?)
- add generic osc/t3d output support to mec-api (MecCallback?)
- better surface mapping
- runtime configuration changes (properties)
- push 2 support
- runtime connect/disconnect of devices

#mec-api , windows support
- perhaps check with MGwin build (allows pthread)
- rtmidi uses pthread, move to juce?
- adapt cmake
- soundplane lib 
- move to use std::thread in mec-app, remove any other pthread dependancy 

