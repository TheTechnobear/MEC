#refactor/generalising
- voices data, a few places where I do similar things to collect the data
- midi generation, create a processor
- osc generation, create a processor



#mec-api , known issues
- voice allocation? (eigenharp looks wrong, in VST)

#mec-api , planned changes - mec-api
- midi/mpe input support
- osc/t3d input support
- add generic midi/mpe output support to mec-api (MecCallback?)
- add generic osc/t3d output support to mec-api (MecCallback?)
- better surface mapping
- runtime configuration changes (properties)
- push 2 support
- runtime connect/disconnect of devices

#mec-api , windows support
- currently pthread could be used with MGWin
- rtmidi uses pthread, move to juce?
- adapt cmake
- soundplane lib 
- move to use std::thread in mec-api 

