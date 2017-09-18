##MEC - Micro Expression Control

The primary goal of this project is to provide a lightweight (micro) environment for expressive controllers. The software is light enough to be used on tiny microcomputers (like the raspberry PI), and yet integrate into different software environments.
One of the inital motivations (that still exists) is to allow controllers like the Eigenharp/Soundplane to operate without the need for a computer.

the deeper motivation is to allow these 'controllers' to become more 'instrument' like, so you can just pickup and play without thinking about software configurations etc.

##Architecture
The core of the project is the mec-api this provides the main functionality which can be utilised by any software. (written in C++ and provided as shared lirbaries).

Building on this core a number of integation 'applications' are provided.

- mec-app,  a standalone console application. (main priority)
- mec-max,  an 'external' for MAX/Msp 
- mec-bela, a project for the Bela platform (see http://bela.io)  
- mec-vst,  a VST/AU which sends 'touches' from mec to VST/AU it hosts.


##Current Platforms

- Mac OSX 10.11
- macOS Sierra
- Linux x86
- Linux Arm  - rPI, BBB, BBB+Bela, Organelle.

(Windows is also planned, but lower priority)


##Documentation

See documents in the [docs sub-folder](/docs).
Documents cover building, installation and running.
The TODO document covers known issues, and current development plans.

##Status
Current status: Under Development
Many things work, but some things have known issues (see docs/todo), technical hurdle and proof of concepts are complete. Implementation of basic functionality (excluding known issue) is near completion.
Quite a few more advanced features are still under active development.

Collaboration is welcome (via pull requests), however please bare in mind, as this is under development, alot of things are still in a fluid state, meaning changes could break proposals - even the direction is not full stable. 
Bug fixing is very useful, but please contact me before making changes api/architectural changes, as undocumented plans exist.

Testing/Usage , under development so you are welcome to use, and if you find issues to report them, please make bugs reports as specific as possible, any general reports I'll just assume will be fixed during my own development testing.
please also check todo, for known issues, and to see if a 'feature' is considered to be complete and a priority.
Im not taking feature requests at the moment (see todo/goals)


##Credits
Id like to thank the following open source projects for helping make this possible:
- oscpack
- rtmidi
- Juce
- cJson
- portAudio 
- libusb

Id like to extend a special heartfelt thank you to **John Lambert/EigenLabs** and **Randy Jones/Madrona Labs**. 
They both made creative/fantastic instruments that I love to play... but also but also had the foresight to open source  their software (used to control them) - thus allowing projects such as these.  It would have been easier for them to guard their 'intellectual property', and I'm very gratefully to them they did not choose this route. 

**Thank You**


