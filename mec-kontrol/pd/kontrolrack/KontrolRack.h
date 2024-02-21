#pragma once

#include "../m_pd.h"

#include <KontrolModel.h>
#include <Parameter.h>
#include <OSCReceiver.h>
#include <OSCBroadcaster.h>

#include "devices/KontrolDevice.h"

#include <memory>
#include <unordered_map>

struct t_KontrolMonitor;

typedef struct _KontrolRack {
    t_object x_obj;
    t_clock *x_clock;

    std::shared_ptr<Kontrol::KontrolModel> model_;
    std::shared_ptr<KontrolDevice> device_;

    long pollCount_;
    std::shared_ptr<Kontrol::OSCReceiver> osc_receiver_;
    std::shared_ptr<Kontrol::OSCBroadcaster> osc_broadcaster_;
    t_symbol* active_module_;
    bool single_module_mode_;
    bool monitor_enable_; // Enable monitoring control changes within PD, and forwarding to KontrolModel.

    std::unordered_map<t_symbol *, t_KontrolMonitor*> *param_monitors_;
} t_KontrolRack;


//define pure data methods
extern "C" {
void KontrolRack_tick(t_KontrolRack *x);
void KontrolRack_free(t_KontrolRack *);
void *KontrolRack_new(t_symbol*, int argc, t_atom *argv);
EXTERN void KontrolRack_setup(void);


void KontrolRack_listen(t_KontrolRack *x, t_floatarg f);
void KontrolRack_connect(t_KontrolRack *x, t_floatarg f);

void KontrolRack_enc(t_KontrolRack *x, t_floatarg f);
void KontrolRack_encbut(t_KontrolRack *x, t_floatarg f);
void KontrolRack_knob1Raw(t_KontrolRack *x, t_floatarg f);
void KontrolRack_knob2Raw(t_KontrolRack *x, t_floatarg f);
void KontrolRack_knob3Raw(t_KontrolRack *x, t_floatarg f);
void KontrolRack_knob4Raw(t_KontrolRack *x, t_floatarg f);
void KontrolRack_midiCC(t_KontrolRack *x, t_floatarg ch, t_floatarg cc, t_floatarg value);
void KontrolRack_key(t_KontrolRack *x, t_floatarg key, t_floatarg value);
void KontrolRack_pgm(t_KontrolRack *x, t_floatarg f);


void KontrolRack_modulate(t_KontrolRack *x, t_symbol* src, t_floatarg bus, t_floatarg value);

void KontrolRack_loadsettings(t_KontrolRack *x, t_symbol *settings);
void KontrolRack_savesettings(t_KontrolRack *x, t_symbol *settings);
void KontrolRack_loadpreset(t_KontrolRack *x, t_symbol *preset);
void KontrolRack_savepreset(t_KontrolRack *x, t_symbol *preset);
void KontrolRack_savecurrentpreset(t_KontrolRack *x);

void KontrolRack_nextpreset(t_KontrolRack *x);
void KontrolRack_prevpreset(t_KontrolRack *x);


void KontrolRack_loadmodule(t_KontrolRack *x, t_symbol *modId, t_symbol* mod);
void KontrolRack_singlemodulemode(t_KontrolRack *x, t_floatarg f);
void KontrolRack_enablemenu(t_KontrolRack *x, t_floatarg f);
void KontrolRack_monitorenable(t_KontrolRack *x, t_floatarg f);

void KontrolRack_test(t_KontrolRack *x, t_floatarg f);

void KontrolRack_getparam(t_KontrolRack* x, t_symbol* modId, t_symbol* paramId, t_floatarg defvalue, t_symbol* sendsym);
void KontrolRack_setparam(t_KontrolRack* x, t_symbol* modId, t_symbol* paramId, t_floatarg value);

void KontrolRack_getsetting(t_KontrolRack* x, t_symbol* setting, t_symbol* defvalue, t_symbol* sendsym);

void KontrolRack_selectpage(t_KontrolRack* x, t_floatarg value);
void KontrolRack_selectmodule(t_KontrolRack* x, t_floatarg value);

void KontrolRack_loadresources(t_KontrolRack *x);

void KontrolRack_setmoduleorder(t_KontrolRack *x,t_symbol* s, int argc, t_atom *argv);
void KontrolRack_instantParam(t_KontrolRack *x, t_floatarg setting);

}

extern void KontrolRack_cleanup(void);
