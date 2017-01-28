## MEC for Max
an external which exposes expressive controllers to MAX

## Status
does not compile yet :) 
I need to fix the link flags, to deal with the dynamic libs, for some reason its not working currently.


## TODO
rationalise everything!
this is based on a max external I wrote for EigenFreeD for the Eigenharp, 
so I simply 'adjusted' it for MEC and moved it to the Max7 sdk.
so I will be updating it to reflect mec better... 
(also I'll be extending the mec-api to provide more integration possibiliies, and standardisation with AU/VST/standalone)


## About
This is built based on the max-api not max-sdk, and the folder structure is from the max-devkit, some features (e.g. CI) are not used yet, but Ive left them in place, in case I want to use them later. 
Currently this CMake setup is not combined into the main MEC CMake setup, this may or may not change


## Prerequisites

To build the externals in this package you will need some form of compiler support on your system. 

* On the Mac this means Xcode (you can get from the App Store for free). 
* On Windows this most likely means some version of Visual Studio (the free versions should work fine).

You will also need to install [CMake](https://cmake.org/download/).


## Building

0. Get the code from Github, or download a zip and unpack it into a folder.
1. In the Terminal or Console app of your choice, change directories (cd) into the folder you created in step 0.
2. `mkdir build` to create a folder with your various build files
3. `cd build` to put yourself into that folder
4. Now you can generate the projects for your choosen build environment:

### Mac 

You can build on the command line using Makefiles, or you can generate an Xcode project and use the GUI to build.

* Xcode: Run `cmake -G Xcode ..` and then run `cmake --build .` or open the Xcode project from this "build" folder and use the GUI.
* Make: Run `cmake ..` and then run `cmake --build .` or `make`.  Note that the Xcode project is preferrable because it is able substitute values for e.g. the Info.plist files in your builds.

### Windows

The exact command line you use will depend on what version of Visual Studio you have installed.  You can run `cmake --help` to get a list of the options available.  Assuming some version of Visual Studio 2013, the commands to generate the projects will look like this:

* 32 bit: `cmake -G "Visual Studio 12" ..`
* 64 bit: `cmake -G "Visual Studio 12 Win64" -DWIN64:Bool=True ..`

Having generated the projects, you can now build by opening the .sln file in the build folder with the Visual Studio app (just double-click the .sln file) or you can build on the command line like this:

`cmake --build . --config Release`


