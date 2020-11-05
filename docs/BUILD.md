# git 
git submodule update --init --recursive

# Dependancies
CMake
libusb

# Building linux (or mac)

    sudo apt install git
    sudo apt install cmake

    // libaries used
    sudo apt-get install libusb-1.0-0-dev
    sudo apt-get install libasound2-dev (linux)
    sudo apt-get install libcairo2-dev (linux) 
    mkdir build
    cd build
    cmake .. 


# Building Mac (with xcode)

    brew install git
    brew install cmake
 
    brew install libusb --universal
    brew install cairo

    mkdir build
    cd build
    cmake .. -GXcode 

    xcodebuild -project mec.xcodeproj -target ALL_BUILD -configuration MinSizeRel

or

    cmake --build .

# Other builds
mec-max, and mec-vst need to be build separately with the project provided

# Building within Sublime Text

you can also build within sublime

    mkdir build
    cd build
    cmake -G "Sublime Text 2 - Unix Makefiles" .. 


# Building on Bela
the bela web ui does not support a subdirectories, so the way install is
  
    mkdir -p ~/projects
    cd ~/projects 
    // git clone MEC.git here
    apt-get install libusb-1.0-0-dev
    apt-get install libcairo2-dev
    mkdir build
    cd build
    cmake .. 
    cd ~/Bela/projects
    ln ~/projects/MEC/mec-bela

this means we now have the mec-bela project in the 'normal' bela projects directory, but it refers to ~/projects/MEC, so you should push from ~/Projects/Mec


# Building on Windows
This is very early days, in fact I mainly compile for Windows just for testing/experiments

currently, build support is only using  MINGW_W64 using pthreads
    
    download and install CMake Win32
    download and install mingw-w64, take default options, including posix/pthreads
    inside mingw-w64, you'll see mingw-w64.bat, run and it will set envionment variables
    change to MEC directory 

    mkdir build
    cd build
    mkdir build
    cmake .. -G "MinGW Makefiles"
    mingw32-make


an alternative, is to use Jetbrains CLion IDE (its very good :) ) 
with this, once you download mingw-w64, from the menu:

     settings->build, execution and deployment -> toolchains
     select MinGW, should say directory you installed into.. select this.


note: currently some device support is disabled under windows, but this will be added soon.
(primarily its missing libusb support)

these build methods all use the mingw pthreads layer rather than being a true native windows build, this will need to be review to check performance and stability.
similary a full native build would probably require a MSVC build, though its unclear if this provides any real advantage to an enduser.


a final alternative is to build using the "Windows Subsystem for Linux"

# Push 2
now requires cairo graphics library, see here
https://cairographics.org/download/


# Eigenharp PICO
the decoder for the pico is not open source, so a dummy implementation is supplied as source.
this will obviously not work though.
however you will find in resources binary versions of the decoder library for various platforms, these should be used in place of the dummy version, built by the make process



# Important note - Kernel Panic on macOS
Mac OSX 10.11+ - Stopping apps
abrutly stopping MEC, will cause the usb driver to create a **kernel panic**.
this is caused byy an issue in the macOS kernel code - so its important MEC is shutdown gracefully even when using Xcode for debugging!

there are two ways to shutdown nicely
OSC

    oscsend localhost 9000 /t3d/command s shutdown

Ctrl-C , ONCE only!
there is also a Ctrl-C handler which will shutdown nicely, but dont keep hammering Ctrl-C to exit quicker, you will cause a panic )



#Disable device support
you can disable the mec devices using the following defines to CMAKE

DISABLE_SOUNDPLANELITE
DISABLE_EIGENHARP
DISABLE_PUSH2
DISABLE_OSCDISPLAY

#enable device
TTUI
ZERORAC
NUI
