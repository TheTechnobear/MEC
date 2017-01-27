#Dependancies
CMake
libusb
homebrew (mac), or you need to install cmake manually 

#Building linux (or mac)

    apt-get install libusb-1.0-0-dev

    mkdir build
    cd build
    cmake .. 


#Building Mac (with xcode)

    brew install cmake

    mkdir build
    cd build
    cmake .. -GXcode 
    xcodebuild -project mec.xcodeproj -target ALL_BUILD -configuration MinSizeRel

#Building within Sublime

you can also building within sublime

    mkdir build
    cd build
    cmake -G "Sublime Text 2 - Unix Makefiles" .. 


#Running
there is a preferences file, called mec.json which needs to be in the current directory when run.
there is an example config in mec/resources

Eigenharps need access to the firmware files (ihx), in firmware directory
Soundplane need access to the SoundplaneAppState.txt, in resources directory 

#Eigenharp PICO
the decoder for the pico is not open source, so a dummy implementation is supplied as source.
this will obviously not work though.
however you will find in eigenharp/resources binary versions of the decoder library for various platforms, these should be used in place of the dummy version, built by the make process



#Important notes
Mac OSX 10.11 - Stopping apps
becareful when stopping under El Capitan (MAC OSX 10.11) , abrutly stopping usb drivers will cause panic
for mec use osc to terminate EVEN inside Xcode!

    oscsend localhost 9000 /tb/mec/command s stop

(there is also a Ctrl-C handler which will shutdown nicely)









