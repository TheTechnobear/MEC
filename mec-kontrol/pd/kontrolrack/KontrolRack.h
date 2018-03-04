#pragma once

#include "../m_pd.h"

#include <KontrolModel.h>
#include <Parameter.h>
#include <OSCReceiver.h>
#include <OSCBroadcaster.h>

#include "devices/KontrolDevice.h"
#include "devices/Organelle.h"

#include <memory>
#include <unordered_map>


typedef struct _KontrolRack {
    t_object x_obj;
    t_clock *x_clock;

    std::shared_ptr<Kontrol::KontrolModel> model_;
    std::shared_ptr<Organelle> device_;

    long pollCount_;
    std::shared_ptr<Kontrol::OSCReceiver> osc_receiver_;
    std::shared_ptr<Kontrol::OSCBroadcaster> osc_broadcaster_;
} t_KontrolRack;


//define pure data methods
extern "C" {
void KontrolRack_tick(t_KontrolRack *x);
void KontrolRack_free(t_KontrolRack *);
void *KontrolRack_new(t_floatarg,t_floatarg);
void KontrolRack_setup(void);

void KontrolRack_test(t_KontrolRack *x);

void KontrolRack_listen(t_KontrolRack *x, t_floatarg f);
void KontrolRack_connect(t_KontrolRack *x, t_floatarg f);

void KontrolRack_enc(t_KontrolRack *x, t_floatarg f);
void KontrolRack_encbut(t_KontrolRack *x, t_floatarg f);
void KontrolRack_knob1Raw(t_KontrolRack *x, t_floatarg f);
void KontrolRack_knob2Raw(t_KontrolRack *x, t_floatarg f);
void KontrolRack_knob3Raw(t_KontrolRack *x, t_floatarg f);
void KontrolRack_knob4Raw(t_KontrolRack *x, t_floatarg f);
void KontrolRack_midiCC(t_KontrolRack *x, t_floatarg cc, t_floatarg value);
void KontrolRack_loadsettings(t_KontrolRack *x, t_symbol *settings);
void KontrolRack_loadpreset(t_KontrolRack *x, t_symbol *preset);

}
