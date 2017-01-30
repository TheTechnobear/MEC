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
- osc generation, create a processor (issue is oscpack, doesnt have a message as such... just a stream, does this add much?... do we need osc output elsewhere?)
- reconsider rtmidi vs juce , rtmidi is dependent on pthread, so perhasp juce

#improvements
- osc/t3d ouput (mec-app) , output periodic framemessage (/t3d/frm)
- osc/t3d input, track /t3d/dr, then look for /t3d/frm cancel voices if not received in time
- config - device class and instances...

#mec-api , known issues
- test eigenharp voice allocation again!
- push2 , I call render in process(), it probably should be in a separate low prio thread.
- consider libusb initialisation, may be multile libusb devices running

#mec-api , planned changes - mec-api
- better surface mapping
- runtime configuration changes (properties)
- push 2 support, using properties/runtime config
- runtime connect/disconnect of devices
- multiple instances of devices
- allow api to say which devices to start... so can run multiple instances using same mec.json

#mec-api , windows support
- perhaps check with MGwin build (allows pthread)
- rtmidi uses pthread, move to juce?
- adapt cmake
- soundplane lib 
- move to use std::thread in mec-app, remove any other pthread dependancy 

