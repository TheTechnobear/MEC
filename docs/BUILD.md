#Dependancies
CMake
libusb

#Building linux (or mac)

    apt-get install libusb-1.0-0-dev
    apt-get install libasound2-dev (linux only)
    mkdir build
    cd build
    cmake .. 


#Building Mac (with xcode)

    brew install cmake
    brew install libusb --universal

    mkdir build
    cd build
    cmake .. -GXcode 

    xcodebuild -project mec.xcodeproj -target ALL_BUILD -configuration MinSizeRel

or

    cmake --build .


#Building within Sublime Text

you can also building within sublime

    mkdir build
    cd build
    cmake -G "Sublime Text 2 - Unix Makefiles" .. 


#Building on Bela
the bela web ui does not support a subdirectories, so the way install is
  
    mkdir -p ~/projects
    cd ~/projects 
    // git clone MEC.git here
    apt-get install libusb-1.0-0-dev
    mkdir build
    cd build
    cmake .. 
    cd ~/Bela/projects
    ln ~/projects/MEC/mec-bela

this means we now have the mec-bela project in the 'normal' bela projects directory, but it refers to ~/projects/MEC
this means any changes, you should push from ~/Projects/Mec

#Running
there is a preferences file, called mec.json which needs to be in the current directory when run.
there is an example config in resources directorys
Eigenharps need access to the firmware files (ihx), in resources directory
Soundplane need access to the SoundplaneAppState.txt, in resources directory 

#Running VST/AU
a) you will need to build mec, and then the mec-vst project separately
b) after building, you need to copy the dynamic libs from ~/build/release/lib to ~/Library/Audio/Plug-Ins/VST/MEC.vst/Frameworks

this is temporary until I sort out a better build/installation procedure


#Eigenharp PICO
the decoder for the pico is not open source, so a dummy implementation is supplied as source.
this will obviously not work though.
however you will find in resources binary versions of the decoder library for various platforms, these should be used in place of the dummy version, built by the make process



#Important notes
Mac OSX 10.11+ - Stopping apps
becareful when stopping under MacOS 10.11+ abrutly stopping usb drivers will cause panic - for mec use osc to terminate EVEN inside Xcode!

    oscsend localhost 9000 /t3d/command s shutdown

(there is also a Ctrl-C handler which will shutdown nicely)









