#Running and installing on Linux

#Tested
- Debian Jessie (Beaglebone)
- Ubuntu 16.04 LTS
- Ubuntu MATE 16.04 raspberry pi

#setup distro for build

    // upgrade distro
    sudo apt-get update
    sudo apt-get upgrade

    // gets new kernel for PI (only)
    sudo rpi-update 


    sudo apt install git
    sudo apt install cmake

    // libaries used
    sudo apt-get install libusb-1.0-0-dev libasound2-dev


#config system

    vi /etc/security/limits.conf

    @audio - rtprio 90
    @audio - memlock unlimited

    sudo adduser yourusername audio 

    sudo cp ../eigenharp/resources/*.rules /etc/udev/rules.d/
    sudo cp ../soundplanelite/resources/*.rules /etc/udev/rules.d/
    sudo cp ../axoloti/resources/*.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules
 
 

#for user (in directory)
     cp ../soundplanelite/resources/SodundplaneAppState.txt
     cp ../mec/resources/mec.json

edit mec.json for input and output devices