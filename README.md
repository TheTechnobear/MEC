##Overview
**MEC - Micro Expression Controller**

The primary goal of this project is to provide a lightweight (micro) environment for expressive controllers. The software is light enough to be used on tiny microcomputers (like the raspberry PI), and yet integrate into different software environments.
One of the inital motivations (that still exists) is to allow controllers like the Eigenharp/Soundplane to operate without the need for a computer.

the deeper motivation is to allow these 'controllers' to become more 'instrument' like, so you can just pickup and play without thinking about software configurations etc.

##Architecture
The core of the project is the mec-api this provides the main functionality which can be utilised by any software. (written in C++ and provided as shared lirbaries).

Building on this core a number of integation 'applications' are provided.

- mec-app,  a standalone console application.
- mec-vst,  a VST/AU which sends 'touches' from mec to VST/AU it hosts.
- mec-max,  an 'external' for MAX/Msp 
- mec-bela, a project for the Bela platform (see http://bela.io)  


##Current Platforms

- Mac OSX 10.11
- macOS Sierra
- Linux x86
- Linux Arm , tested on Raspberry PI2 and BBB including Bela

(Windows is also planned, most of the software already runs on Windows)


##Building and Running

please see the various documents in the docs sub-folder

##Credits
Id like to thank the following open source projects for helping make this possible:
- oscpack
- rtmidi
- Juce
- cJson
- portAudio 
- libusb

Id also like to extend a special heartfelt thank you to **John Lambert/EigenLabs** and **Randy Jones/Madrona Labs**. 
They both made creative/fantastic instruments that I love to play... but also but also had the foresight to open source  their software (used to control them) - thus allowing project such as mine. 
It would have been easier for them to guard their 'intellectual property', and I'm very gratefully to them they did not choose this route. 

**Thank You**


