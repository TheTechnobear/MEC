todo items are included in TODO.txt

this file is more general notes,ideas and current status

# Project
SoundplaneLite - stripped down soundplane software, all in one folder (in use)
Soundplane - modified soundplane software, but with ML folder structure (not in use currently )
Eigenharp -  including EigenFreeD, latest version of EigenFreeD, which will cease to exist in other forms(!?) (have to think about Max external here)
Push 2 - controlling code for Push2, currently only test stuff
Mec - code which binds controller software into a useable form i.e. with Midi/Osc output
external - external code i.e. projects like oscpack/libusb/rtmidi etc


## Input devices

*Eigenharp*
pico basically works with midi, need to test alpha again, and also have some kind of dimensions code
... needs scale mapping

*Soundplane*
two implementations, a low level one (soundplane_tt_proc), and one usng the soundplane model,
I want to use the soundplanemodel one, as this includes calibration, and means the soundplaneapp.txt file is useable
however I want to have an output, that targets the MEC outputs.

*Push 2*
only basic tests done, not integrated
idea is to support a push1 compatibity mode, and also general controller mode.
want to be able to display parameters etc on screen and also use pads for playing/sequencing

*Midi (keyboard)**
low priority, but idea is to support a midi input

##Output devices

*Midi*

*OSC*
not implemented currently, basics are in eigenharp, but I will probably adopt T3D

