#Architecture

MEC running on Beaglebone Black with Bela connected to an Eigenharp (Alpha or Pico), Soundplane and Push 2. This sends MPE messages to the Axoloti Core(s) running synthesis patches, and also FX.
The intention is to run both the BBB and Axoloti Cores off a USB battery pack.

Connections, USB powered hub powered by battery pack. Hub in turn supplied power to BBB and Axoloti cores. battery pack will also power push2.
BBB with be USB host to the usb hub.

#MicroExpression Controller Project (MEC)

The goal of MEC is to provide an lightweight applicaiton, that can control expressive controllers, 
in particular the Eigenharp and Soundplane, and on something with as limited resources as a Raspberry PI or BeagleBone Black. also to facilate this, and its crossplatform aims, it needs to have few dependancies, and all open source.

Push2 will provide both a control surface (pads) and also a visual feedback thru the LCD for controllering parmeters etc.

#Current Platforms

- Mac OSX 10.11
- macOS Sierra
- Linux x86
- Linux Arm , tested on Raspberry PI2 and BBB including Bela

Primary target is BBB with Bela.

#Building MEC

See BUILD.md in docs folders , but basically its:
mkdir build
cd build
cmake ..
make 


#Eigenharp PICO
currently the pico implementation is 'stubbed' out, i.e. non-operational, as the decoder is not open source,
this will be 'addressed' in a future release



#MEC important note

Mac OSX 10.11 (only, macOS Sierra ok) - Stopping apps
be careful when stopping under Mac OS post 10.11 , abrutly stopping usb drivers will cause kernel panic, due to a bug in macOS
for mec use osc to terminate EVEN inside Xcode!

    oscsend localhost 9000 /tb/mec/command s stop

(there is also a Ctrl-C handler which will shutdown nicely)
