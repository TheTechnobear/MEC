#include <push2lib/push2lib.h>


#include "mec_app.h"

// #include "midi_output.h"
#include <mec_prefs.h>

void *push2_proc(void * arg)
{
    LOG_0("push2_proc start" );

    MecPreferences prefs(arg);

    MecPreferences outprefs(prefs.getSubTree("outputs"));
    bool midiEnabled = outprefs.exists("midi");
    bool oscEnabled = outprefs.exists("osc");

    Push2API::Push2 push2;

    push2.init();

    // 'test for Push 1 compatibility mode'
    int row = 2;
    push2.clearDisplay();
    push2.drawText(row, 10, "...01234567891234567......");
    push2.clearRow(row);
    push2.p1_drawCell(row, 0, "01234567891234567");
    push2.p1_drawCell(row, 1, "01234567891234567");
    push2.p1_drawCell(row, 2, "01234567891234567");
    push2.p1_drawCell(row, 3, "01234567891234567");


    if (midiEnabled || oscEnabled) {

        if (oscEnabled) {
            MecPreferences oscprefs(outprefs.getSubTree("osc"));
        }
        if (midiEnabled) {
            MecPreferences midiprefs(outprefs.getSubTree("midi"));
        }

        pthread_mutex_lock(&waitMtx);
        while (keepRunning)
        {
            push2.render();
            struct timespec ts;
            getWaitTime(ts, 1000);
            pthread_cond_timedwait(&waitCond, &waitMtx, &ts);
        }
        pthread_mutex_unlock(&waitMtx);
    }

    push2.deinit();

    LOG_0("push2_proc stop" );
    pthread_exit(NULL);
}


