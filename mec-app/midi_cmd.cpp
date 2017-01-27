#include <unistd.h>
#include <string.h>

#include <pthread.h>

#include <cstdlib>
#include <RtMidi.h>

#include "mec_app.h"
#include <mec_prefs.h>


void mycallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    unsigned int nBytes = message->size();
    for ( unsigned int i = 0; i < nBytes; i++ )
        LOG_1( "Byte " << i << " = " << (int)message->at(i) << ", ");
    if ( nBytes > 0 )
        LOG_1( "stamp = " << deltatime );
}



void *midi_proc(void *arg)
{
    LOG_1("midi_command_proc start");

    MecPreferences prefs(arg);
    RtMidiIn *midiin = new RtMidiIn();

    std::string portname = prefs.getString("device");

    bool found = false;
    for (int i = 0; i < midiin->getPortCount() && !found; i++) {
        if (portname.compare(midiin->getPortName(i)) == 0) {
            try {
                midiin->openPort(i);
                found = true;
                LOG_1("Midi input opened :" << portname);
            } catch (RtMidiError  &error) {
                LOG_1("Midi input open error:" << error.what());
                goto end;
            }
        }
    }
    if (!found) {
        LOG_0("input port not found : [" << portname << "]");
        LOG_0("available ports:");
        for (int i = 0; i < midiin->getPortCount(); i++) {
           LOG_0("[" << midiin->getPortName(i) << "]");
        }
        goto end;
    }



    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the
    // queue.
    midiin->setCallback( &mycallback );
    // ignore sysex, timing, or active sensing messages.
    midiin->ignoreTypes( true, true, true );
    LOG_1( "\nReading MIDI input ...for 60 seconds");
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
    LOG_1("midi_command_proc stop" );
    pthread_exit(NULL);
}


