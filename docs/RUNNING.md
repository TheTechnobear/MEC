##configuration
the sample mec.json has many enries, most are not required
I have simply included all entries, with thier default.

json files are fairly simply but its worth using an editor with json syntax checking, the most common mistake is not including commas in lists, or including a comment for the last entry... very easy to do :)
json does not allow commenting out of lines, so instead where I want to comment out and entry (e.g. to show some options) , I prefix it with an underscore... but you should remember it still a valid json line, so will need a comma at the end 


#other useful tools
macos - homebrew :)

    brew install liblo 

provides oscdump and oscsend

#Running macOS

copy resources and then adapt mec.json

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



##Setup for linux distro for build

    // upgrade distro
    sudo apt-get update
    sudo apt-get upgrade

    // gets new kernel for PI (only)
    sudo rpi-update 


    sudo apt install git
    sudo apt install cmake

    // libaries used
    sudo apt-get install libusb-1.0-0-dev libasound2-dev


##Config system
note: this may vary with distros, this is for debian based distros

    vi /etc/security/limits.conf

    @audio - rtprio 90
    @audio - memlock unlimited

    sudo adduser yourusername audio 

    sudo cp resources/*.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules
 
