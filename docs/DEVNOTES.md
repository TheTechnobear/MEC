todo items are included in TODO.txt

this file is more general notes,ideas and current status

# Project
mec-api - a framework for integrating expressive devices
mec-app - standalone app, built on mec-api
mec-vst - VST/AU, built on mec-api
mec-app - bela app, built on mec-api
mec-max - max external, built on mec-api

SoundplaneLite - stripped down soundplane software, all in one folder (in use)
cease to exist in other forms(!?) (have to think about Max external here)
Push 2 - controlling code for Push2, currently only test stuff
Mec - code which binds controller software into a useable form i.e. with Midi/Osc output
external - external code i.e. projects like oscpack/libusb/rtmidi etc


## Input devices

*Eigenharp*
extends  EigenFreeD, which in turn is built on source code from Eigenlabs.
EigenD code, stripped to its essence, with a changes and also linux support

*Soundplane*
built on soundplanelite, which is a cut down and extended version of soundplane code from Madrona labs
two implementations: 
- a low level one (soundplane_tt_proc), 
- one usng the soundplane model
currently we use the soundplane model implementation with a new 

*Push 2*
only test code for now, showing display, and passing thru midi
idea is to support a push1 compatibity mode, and also general controller mode.
want to be able to display parameters etc on screen and also use pads for playing/sequencing

*MIDI*
supports MPE input, and also 'vanila midi' 

*OSC - T3D*
to do

*OSC - Command*
idea is to provide a remove command interface to MEC via OSC


##Output devices

currently this is in the mec apps , the idea is to bring some common code into mec-api (without imposing an api) to support common mapping for midi and osc, and potentially other  output formats... this will be exposed as 'processors'

*Midi*
primary target, built on rtmidi or JUCE for VST/AU

*OSC*
now in t3d format


##Mec API 
provides interface to underlying input devices, with a common callback interface. the app using the mec api registers callbacks and then calls process().
the callbacks are processed syncronoushly to the process() call, which is expected to be in the audio thread (i.e no blocking etc)


