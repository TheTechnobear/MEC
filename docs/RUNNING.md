##configuration
All configuration is provide by a json file, by default this is mec.json in the current directory, or you can specify one on as the first argument on the command line

resources/mec.json is a sample configuration file, that will need to be adapted to your needs.

you will see it has large number of entries, most of these are *not* required - they are provided to document the default that is used by the code.

json files are fairly simply but its worth using an editor with json syntax checking, the most common mistake is not including commas in lists, or including a comment for the last entry... very easy to do :)
json does not allow commenting out of lines - by convention, where I want to comment out an entry (e.g. to show options) , I prefix the 'key name' with an underscore. 
If you are uncommenting , or commenting out, the line must still be valid json (again take are with commas at line ends) 


#Running macOS

copy resources and then adapt mec.json

#Running VST/AU
a) you will need to build mec, and then the mec-vst project separately
b) after building, you need to copy the dynamic libs from ~/build/release/lib to ~/Library/Audio/Plug-Ins/VST/MEC.vst/Frameworks

this is temporary until I sort out a better build/installation procedure


#Important note - Kernel Panic on macOS
Mac OSX 10.11+ - Stopping apps
abrutly stopping MEC, will cause the usb driver to create a **kernel panic**.
this is caused byy an issue in the macOS kernel code - so its important MEC is shutdown gracefully even when using Xcode for debugging!

there are two ways to shutdown nicely
OSC

    oscsend localhost 9000 /t3d/command s shutdown

Ctrl-C , ONCE only!
there is also a Ctrl-C handler which will shutdown nicely, but dont keep hammering Ctrl-C to exit quicker, you will cause a panic )



##Tested

- 10.12 macOS sierra

#Running windows

not available yet!

#Running Linux

copy resources and then adapt mec.json

##Tested

- Debian Jessie (Beaglebone)
- Ubuntu 16.04 LTS
- Ubuntu MATE 16.04 raspberry pi
- Rasbian Stretch (rPI3)
- Organelle (arch linux/arm)
- Xenomai (Beaglebone black + bela)

##Setup for linux distro for build

    // upgrade distro
    sudo apt-get update
    sudo apt-get upgrade

    // gets new kernel for PI (only)
    sudo rpi-update 



##Config system
note: this may vary with distros, this is for debian based distros

    vi /etc/security/limits.conf

    @audio - rtprio 90
    @audio - memlock unlimited

    sudo adduser yourusername audio 

    sudo cp resources/*.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules


 
#other useful tools
for testing, its quite useful to use osc, for this i use oscdump to capture messages from mec, and oscsend to sent osc messages to mec.
under macos,  install via homebrew, using brew install liblo
under linux, install liblo ith package manager 
