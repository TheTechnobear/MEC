This document contains general notes on development strategy, and status
More detailed information on status, known issues, priorities is contained in todo.md

# Project
mec-api - a framework for integrating expressive devices
mec-app - standalone app, built on mec-api
mec-vst - VST/AU, built on mec-api
mec-app - bela app, built on mec-api
mec-max - max external, built on mec-api
mec-kontrol - patch and parameter system, also with pd support

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
Initial integration with new parameters system (Kontrol), also some navigation.
this is going to need refactoring soon, as full navigation needs the push 2 to have moves,
and the OLED/Pad colours all need to reflect the mode
this currently inherit from mididevice

*MIDI*
supports MPE input, and also 'vanila midi' 

*OSC - T3D*
to do

*OSC - Command*
idea is to provide a remove command interface to MEC via OSC


## Output devices

currently this is in the mec apps , the idea is to bring some common code into mec-api (without imposing an api) to support common mapping for midi and osc, and potentially other  output formats... this will be exposed as 'processors'

*Midi*
primary target, built on rtmidi or JUCE for VST/AU

*OSC*
now in t3d format

** new ** 
Ive justed added ouptut to mec_mididevice, this was primarily done for Push2, where it needs to be able to output midi to itself to update pad colours etc.... the question is, is this a more general use? or is this just an implementation detail like oled, or leds on an eigenharp?

## Mec API 
provides interface to underlying input devices, with a common callback interface. the app using the mec api registers callbacks and then calls process().
the callbacks are processed syncronoushly to the process() call, which is expected to be in the audio thread (i.e no blocking etc)


## MEC Kontrol
still in development , see mec-kontrol/devnotes.md