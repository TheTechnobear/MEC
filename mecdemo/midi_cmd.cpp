#include <iostream>
#include <unistd.h>
#include <string.h>

#include <pthread.h>

#include <cstdlib>
#include <RtMidi.h>

#include "mec.h"
#include <mec_prefs.h>


void mycallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    unsigned int nBytes = message->size();
    for ( unsigned int i = 0; i < nBytes; i++ )
        LOG_1(std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";)
    if ( nBytes > 0 )
        LOG_1(std::cout << "stamp = " << deltatime << std::endl;)
}



void *midi_proc(void *arg)
{
    LOG_0(std::cout  << "midi_command_proc start" << std::endl;)

    MecPreferences prefs(arg);
    RtMidiIn *midiin = new RtMidiIn();

    std::string portname = prefs.getString("device");

    bool found = false;
    for (int i = 0; i < midiin->getPortCount() && !found; i++) {
        if (portname.compare(midiin->getPortName(i)) == 0) {
            try {
                midiin->openPort(i);
                found = true;
                LOG_0(std::cout << "Midi input opened :" << portname << std::endl;)
            } catch (RtMidiError  &error) {
                error.printMessage();
                goto end;
            }
        }
    }
    if (!found) {
        std::cerr   << "input port not found : [" << portname << "]" << std::endl
                    << "available ports : " << std::endl;
        for (int i = 0; i < midiin->getPortCount(); i++) {
            std::cerr << "[" << midiin->getPortName(i) << "]" << std::endl;
        }
        goto end;
    }



    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the
    // queue.
    midiin->setCallback( &mycallback );
    // ignore sysex, timing, or active sensing messages.
    midiin->ignoreTypes( true, true, true );
    LOG_1(std::cout << "\nReading MIDI input ...for 60 seconds\n";)
    // Clean up

    pthread_mutex_lock(&waitMtx);
    while (keepRunning)
    {
        struct timespec ts;
        getWaitTime(ts, 60 * 1000);
        pthread_cond_timedwait(&waitCond, &waitMtx, &ts);
    }
    pthread_mutex_unlock(&waitMtx);

    delete midiin;
    return 0;

end:
    LOG_0(std::cout  << "midi_command_proc stop" << std::endl;)
    pthread_exit(NULL);
}


