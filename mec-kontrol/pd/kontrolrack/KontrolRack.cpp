#include "KontrolRack.h"


/*****
represents the device interface, of which there is always one
******/

static t_class *KontrolRack_class;

class SendBroadcaster : public Kontrol::KontrolCallback {
public:
    //Kontrol::KontrolCallback
    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override { ; }

    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
              const Kontrol::Page &) override { ; }

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override { ; }

    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override;
};

// puredata methods implementation - start

static const unsigned OSC_POLL_FREQUENCY = 1; // 10ms
static const unsigned DEVICE_POLL_FREQUENCY = 1; // 10ms
static const unsigned OSC_PING_FREQUENCY_SEC = 5;
static const unsigned OSC_PING_FREQUENCY = (OSC_PING_FREQUENCY_SEC * (1000/10) ) ; // 5 seconds


// see https://github.com/pure-data/pure-data/blob/master/src/x_time.c
static const float TICK_MS = 1.0f * 10.0f; // 10.ms

/// main PD methods
void KontrolRack_tick(t_KontrolRack *x) {
    x->pollCount_++;
    if (x->osc_receiver_ && x->pollCount_ % OSC_POLL_FREQUENCY == 0) {
        x->osc_receiver_->poll();
    }

    if (x->device_ && x->pollCount_ % DEVICE_POLL_FREQUENCY == 0) {
        x->device_->poll();
    }

    if (x->osc_broadcaster_ && x->osc_receiver_
        && x->pollCount_ % OSC_PING_FREQUENCY == 0) {
        x->osc_broadcaster_->sendPing(x->osc_receiver_->port());
    }

    clock_delay(x->x_clock, 1);
}


void KontrolRack_free(t_KontrolRack *x) {
    clock_free(x->x_clock);
    x->model_->clearCallbacks();
    if (x->osc_receiver_) x->osc_receiver_->stop();
    x->osc_receiver_.reset();
    x->device_.reset();
    // Kontrol::ParameterModel::free();
}

void *KontrolRack_new(t_floatarg serverport, t_floatarg clientport) {
    t_KontrolRack *x = (t_KontrolRack *) pd_new(KontrolRack_class);


    x->osc_receiver_ = nullptr;

    x->pollCount_ = 0;
    x->model_ = Kontrol::KontrolModel::model();

    x->model_->createLocalRack((unsigned int) clientport);

    x->device_ = std::make_shared<Organelle>();
    x->device_->init();

    x->device_->currentRack(x->model_->localRackId());

    x->model_->addCallback("pd.send", std::make_shared<SendBroadcaster>());
    x->model_->addCallback("pd.dev", x->device_);

    if (clientport) KontrolRack_listen(x, clientport);
    KontrolRack_connect(x, serverport);

    x->x_clock = clock_new(x, (t_method) KontrolRack_tick);
    clock_setunit(x->x_clock, TICK_MS, 0);
    clock_delay(x->x_clock, 1);

    return (void *) x;
}

void KontrolRack_setup(void) {
    KontrolRack_class = class_new(gensym("KontrolRack"),
                                  (t_newmethod) KontrolRack_new,
                                  (t_method) KontrolRack_free,
                                  sizeof(t_KontrolRack),
                                  CLASS_DEFAULT,
                                  A_DEFFLOAT,
                                  A_DEFFLOAT,
                                  A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_listen, gensym("listen"),
                    A_DEFFLOAT, A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_connect, gensym("connect"),
                    A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_knob1Raw, gensym("knob1Raw"),
                    A_DEFFLOAT, A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_knob2Raw, gensym("knob2Raw"),
                    A_DEFFLOAT, A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_knob3Raw, gensym("knob3Raw"),
                    A_DEFFLOAT, A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_knob4Raw, gensym("knob4Raw"),
                    A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_enc, gensym("enc"),
                    A_DEFFLOAT, A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_encbut, gensym("encbut"),
                    A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_midiCC, gensym("midiCC"),
                    A_DEFFLOAT, A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_key, gensym("key"),
                    A_DEFFLOAT, A_DEFFLOAT, A_NULL);

    // class_addmethod(KontrolRack_class,
    //                 (t_method) KontrolRack_vol, gensym("vol"),
    //                 A_DEFFLOAT, A_NULL);
    // class_addmethod(KontrolRack_class,
    //                 (t_method) KontrolRack_expRaw, gensym("expRaw"),
    //                 A_DEFFLOAT, A_NULL);
    // class_addmethod(KontrolRack_class,
    //                 (t_method) KontrolRack_fsRaw, gensym("fsRaw"),
    //                 A_DEFFLOAT, A_NULL);



    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_loadsettings, gensym("loadsettings"),
                    A_DEFSYMBOL, A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_loadpreset, gensym("loadpreset"),
                    A_DEFSYMBOL, A_NULL);


    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_test, gensym("test"),
                    A_NULL);
}

void KontrolRack_test(t_KontrolRack *x) {
    t_pd *sendobj = gensym("pd-slot4")->s_thing;
    if (!sendobj) {
        post("send to ; failed");
        return;
    }

    {
        t_atom a[5];
        SETSYMBOL(&a[0], gensym("obj"));
        SETFLOAT(&a[1], 100);
        SETFLOAT(&a[2], 100);
        SETSYMBOL(&a[3], gensym("pd"));
        SETSYMBOL(&a[4], gensym("slot1"));
        pd_forwardmess(sendobj, 5, a);
    }
}


void KontrolRack_connect(t_KontrolRack *x, t_floatarg f) {
    if (f > 0) {
        std::string host = "127.0.0.1";
        unsigned port = (unsigned) f;
        std::string srcId = host + ":" + std::to_string(port);
        std::string id = "pd.osc:" + srcId;
        x->model_->removeCallback(id);
        auto p = std::make_shared<Kontrol::OSCBroadcaster>(
                Kontrol::ChangeSource(Kontrol::ChangeSource::REMOTE, srcId),
                OSC_PING_FREQUENCY_SEC,
                false);
        if (p->connect(host, port)) {
            post("client connected %s", id.c_str());
            x->model_->addCallback(id, p);
            x->osc_broadcaster_ = p;
            if (x->osc_receiver_) {
                x->osc_broadcaster_->sendPing(x->osc_receiver_->port());
            }
        }
    }
}

void KontrolRack_listen(t_KontrolRack *x, t_floatarg f) {
    if (f > 0) {
        auto p = std::make_shared<Kontrol::OSCReceiver>(x->model_);
        if (p->listen((unsigned) f)) {
            x->osc_receiver_ = p;
        }
    } else {
        x->osc_receiver_.reset();
    }
}

void KontrolRack_enc(t_KontrolRack *x, t_floatarg f) {
    if (x->device_) x->device_->changeEncoder(0, f);
}

void KontrolRack_encbut(t_KontrolRack *x, t_floatarg f) {
    if (x->device_) x->device_->encoderButton(0, (bool) f);
}

void KontrolRack_knob1Raw(t_KontrolRack *x, t_floatarg f) {
    if (x->device_) x->device_->changePot(0, f);
}

void KontrolRack_knob2Raw(t_KontrolRack *x, t_floatarg f) {
    if (x->device_) x->device_->changePot(1, f);
}

void KontrolRack_knob3Raw(t_KontrolRack *x, t_floatarg f) {
    if (x->device_) x->device_->changePot(2, f);
}

void KontrolRack_knob4Raw(t_KontrolRack *x, t_floatarg f) {
    if (x->device_) x->device_->changePot(3, f);
}

void KontrolRack_midiCC(t_KontrolRack *x, t_floatarg cc, t_floatarg value) {
    if (x->device_) x->device_->midiCC((unsigned) cc, (unsigned) value);
}

void KontrolRack_key(t_KontrolRack *x, t_floatarg key, t_floatarg value) {
    if (x->device_) x->device_->keyPress((unsigned) key, (unsigned) value);
}


void SendBroadcaster::changed(Kontrol::ChangeSource,
                              const Kontrol::Rack &rack,
                              const Kontrol::Module &module,
                              const Kontrol::Parameter &param) {

    t_pd *sendobj = gensym(param.id().c_str())->s_thing;


    if (!sendobj) {
        post("send to %s failed", param.id().c_str());
        return;
    }

    t_atom a;
    switch (param.current().type()) {
        case Kontrol::ParamValue::T_Float : {
            SETFLOAT(&a, param.current().floatValue());
            break;
        }
        case Kontrol::ParamValue::T_String:
        default: {
            SETSYMBOL(&a, gensym(param.current().stringValue().c_str()));
            break;
        }
    }
    pd_forwardmess(sendobj, 1, &a);
}



void KontrolRack_loadsettings(t_KontrolRack *x, t_symbol *settings) {
    if(settings!= nullptr && settings->s_name != nullptr && strlen(settings->s_name)>0) {
        auto rack = Kontrol::KontrolModel::model()->getLocalRack();
        if (rack) {
            std::string settingsId = settings->s_name;
            post("loading settings : %s", settings->s_name);
            rack->loadSettings(settingsId + "-rack.json");
            rack->dumpSettings();
        } else {
            post("No local rack found");
        };
    }
}

void KontrolRack_loadpreset(t_KontrolRack *x, t_symbol *preset) {
    if(preset!= nullptr && preset->s_name != nullptr && strlen(preset->s_name)>0) {
        auto rack = Kontrol::KontrolModel::model()->getLocalRack();
        if (rack) {
            std::string presetId = preset->s_name;
            post("loading preset : %s", preset->s_name);
            rack->applyPreset(presetId);
            rack->dumpCurrentValues();
        }
    } else {
        post("No local rack found");
    }
}

// puredata methods implementation - end
