#UBUNTU MATE 16.04 raspberry pi

#setup distro for build

    // upgrade distro
    sudo apt-get update
    sudo apt-get upgrade

    // gets new kernel
    sudo rpi-update 


    sudo apt install git
    sudo apt install cmake

    // libaries used
    sudo apt-get install libusb-1.0-0-dev
    sudo apt-get install libasound2-dev


#config system

    vi /etc/security/limits.conf

    @audio - rtprio 90
    @audio - memlock unlimited

    sudo adduser yourusername audio 

    sudo cp ../eigenharp/resources/69-eigenharp.rules /etc/udev/rules.d/
    sudo cp ../soundplanelite/resources/59-soundplane.rules /etc/udev/rules.d/


#for user (in directory)
     cp ../soundplanelite/resources/SodundplaneAppState.txt
     cp ../mec/resources/mec.json

edit mec.json for input and output devices