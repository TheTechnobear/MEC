# current goal/milestone
- ensure minimum use-case is possible = working on all devices, with raw mapping, with lights/text control. 
- does not include: scales, splits,

# current priorities
(in order)
- acheive feature parity, api integration for all devices
- add ability to drives leds for eigenharps via osc
- add ability to drive push 2 parameters/display via osc 
- packaging of mec-app

# mec-api , known issues
- push2 , I call render in process(), it probably should be in a separate low prio thread.
- consider libusb initialisation, may be multile libusb devices running

# mec-api , planned changes - mec-api
- eh led control via osc
- push 2 display via osc
- better surface mapping
- runtime connect/disconnect of devices
- multiple instances of devices
- allow api to say which devices to start... so can run multiple instances using same mec.json
- runtime configuration changes (properties)        (mec-kontrol will implement this)
- push 2 support, using properties/runtime config   (mec-kontrol will implement this)

# mec-api , windows support
- perhaps check with MGwin build (allows pthread)
- rtmidi uses pthread, move to juce?
- adapt cmake
- soundplane lib 
- move to use std::thread in mec-app, remove any other pthread dependancy 

# build issues
- brew install issue on mac, as no longer supply universal binary, build libusb?
- properly package some of the mec apps (some need manual intervention)
- mec-app rpath
- mec-vst need to copy dylibs into Frameworks
- mec-max need to sort out rpath for external, and package dylibs
- consider a top level script to build everything (perhaps options depending on platform being run on... e.g. no point in bela on windows)
- remove OSAtomic*  errors from eigenfreed/eigend, use std::atomic_* instead

# work in progress
- scales
- splits/combining of surfaces
- mec-kontrol

# refactor/generalising
- voices data, a few places where I do similar things to collect the data
- osc generation, create a processor (issue is oscpack, doesnt have a message as such... just a stream, does this add much?... do we need osc output elsewhere?)
- reconsider rtmidi vs juce , rtmidi is dependent on pthread, so perhaps juce

# improvements
- osc/t3d ouput (mec-app) , output periodic framemessage (/t3d/frm)
- osc/t3d input, track /t3d/dr, then look for /t3d/frm cancel voices if not received in time
- config - device class and instances...

# other
- document configuration file/setup
- improve existing docs

# device specific to do
## soundplane
- upgrade to new touch tracker (when stable/available)

## pico
- mode keys
- windows/threshold/warmup for strip(s)/breath

## tau
- test key layout due to 'odd shape'
- mode keys
- windows/threshold/warmup for strip(s)/breath

## alpha
- windows/threshold/warmup for strip(s)/breath

## push2 
- refactor to allow for different control/navigation modes

