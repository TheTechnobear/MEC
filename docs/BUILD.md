#Dependancies
CMake
libusb

#Building linux (or mac)

    sudo apt install git
    sudo apt install cmake

    // libaries used
    sudo apt-get install libusb-1.0-0-dev
    sudo apt-get install libasound2-dev (linux only)
    mkdir build
    cd build
    cmake .. 


#Building Mac (with xcode)

    brew install git
    brew install cmake
 
    brew install libusb --universal

    mkdir build
    cd build
    cmake .. -GXcode 

    xcodebuild -project mec.xcodeproj -target ALL_BUILD -configuration MinSizeRel

or

    cmake --build .

#Other builds
mec-max, and mec-vst need to be build separately with the project provided

#Building within Sublime Text

you can also build within sublime

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

this means we now have the mec-bela project in the 'normal' bela projects directory, but it refers to ~/projects/MEC, so you should push from ~/Projects/Mec


#Eigenharp PICO
the decoder for the pico is not open source, so a dummy implementation is supplied as source.
this will obviously not work though.
however you will find in resources binary versions of the decoder library for various platforms, these should be used in place of the dummy version, built by the make process



#Important note - Kernel Panic on macOS
Mac OSX 10.11+ - Stopping apps
abrutly stopping MEC, will cause the usb driver to create a **kernel panic**.
this is caused byy an issue in the macOS kernel code - so its important MEC is shutdown gracefully even when using Xcode for debugging!

there are two ways to shutdown nicely
OSC

    oscsend localhost 9000 /t3d/command s shutdown

Ctrl-C , ONCE only!
there is also a Ctrl-C handler which will shutdown nicely, but dont keep hammering Ctrl-C to exit quicker, you will cause a panic )






