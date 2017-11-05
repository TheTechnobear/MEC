#include "KontrolRack.h"

#include "devices/Organelle.h"

#include <ip/UdpSocket.h>
#include <osc/OscOutboundPacketStream.h>

#include <string>
#include <algorithm>
#include <limits>

/*****
represents the device interface, of which there is always one
******/

static t_class *KontrolRack_class;

class SendBroadcaster : public Kontrol::KontrolCallback {
public:
    //Kontrol::KontrolCallback
    virtual void rack(Kontrol::ParameterSource, const Kontrol::Rack &) { ; }

    virtual void module(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &) { ; }

    virtual void page(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                      const Kontrol::Page &) { ; }

    virtual void param(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                       const Kontrol::Parameter &) { ; }

    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                         const Kontrol::Parameter &);
};

class ClientHandler : public Kontrol::KontrolCallback {
public:
    ClientHandler(t_KontrolRack *x) : x_(x) { ; }

    //Kontrol::KontrolCallback
    virtual void rack(Kontrol::ParameterSource, const Kontrol::Rack &);

    virtual void module(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &) { ; }

    virtual void page(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                      const Kontrol::Page &) { ; }

    virtual void param(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                       const Kontrol::Parameter &) { ; }

    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                         const Kontrol::Parameter &) { ; }

private:
    t_KontrolRack *x_;
};


class DeviceHandler : public Kontrol::KontrolCallback {
public:
    DeviceHandler(t_KontrolRack *x) : x_(x) { ; }

    //Kontrol::KontrolCallback
    virtual void rack(Kontrol::ParameterSource, const Kontrol::Rack &) { ; }

    virtual void module(Kontrol::ParameterSource src, const Kontrol::Rack &rack, const Kontrol::Module &module) {
        if (src == Kontrol::PS_LOCAL) {
            if (x_->device_ && !(x_->device_->currentModule().empty())) {
//            post(rack.id().c_str());
                x_->device_->currentRack(rack.id());
                x_->device_->currentModule(module.id());
            }
        }
    }

    virtual void page(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                      const Kontrol::Page &) { ; }

    virtual void param(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                       const Kontrol::Parameter &) { ; }

    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack &, const Kontrol::Module &,
                         const Kontrol::Parameter &) { ; }

private:
    t_KontrolRack *x_;
};

// puredata methods implementation - start

// helpers

static t_pd *get_object(const char *s) {
    t_pd *x = gensym(s)->s_thing;
    return x;
}


static void sendPdMessage(const char *obj, float f) {
    t_pd *sendobj = get_object(obj);
    if (!sendobj) {
        post("KontrolRack~::sendPdMessage to %s failed", obj);
        return;
    }

    t_atom a;
    SETFLOAT(&a, f);
    pd_forwardmess(sendobj, 1, &a);
}

static const int OSC_POLL_FREQUENCY = 1;
static const int DEVICE_POLL_FREQUENCY = 1;
static const unsigned TICK_MS = 1000 / 25;

/// main PD methods
void KontrolRack_tick(t_KontrolRack *x) {
    x->pollCount_++;
    if (x->osc_receiver_ && x->pollCount_ % OSC_POLL_FREQUENCY == 0) {
        x->osc_receiver_->poll();
    }

    if (x->device_ && x->pollCount_ % DEVICE_POLL_FREQUENCY == 0) {
        x->device_->poll();
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

void *KontrolRack_new(t_floatarg serverport t_floatarg clientport) {
    t_KontrolRack *x = (t_KontrolRack *) pd_new(KontrolRack_class);


    x->osc_receiver_ = nullptr;

    x->pollCount_ = 0;
    x->model_ = Kontrol::KontrolModel::model();

    x->model_->createLocalRack(port);

    x->device_ = std::make_shared<Organelle>();
    x->device_->init();

    x->device_->currentRack(x->model_->localRackId());

    x->model_->addCallback("pd.send", std::make_shared<SendBroadcaster>());
    x->model_->addCallback("pd.client", std::make_shared<ClientHandler>(x));
    x->model_->addCallback("pd.dev", x->device_);

    KontrolRack_listen(x, clientport);
    KontrolRack_connect(x,serverport);

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

    // class_addmethod(KontrolRack_class,
    //                 (t_method) KontrolRack_tilde_auxRaw, gensym("auxRaw"),
    //                 A_DEFFLOAT, A_NULL);
    // class_addmethod(KontrolRack_class,
    //                 (t_method) KontrolRack_tilde_vol, gensym("vol"),
    //                 A_DEFFLOAT, A_NULL);
    // class_addmethod(KontrolRack_class,
    //                 (t_method) KontrolRack_tilde_expRaw, gensym("expRaw"),
    //                 A_DEFFLOAT, A_NULL);
    // class_addmethod(KontrolRack_class,
    //                 (t_method) KontrolRack_tilde_fsRaw, gensym("fsRaw"),
    //                 A_DEFFLOAT, A_NULL);
    // class_addmethod(KontrolRack_class,
    //                 (t_method) KontrolRack_tilde_notesRaw, gensym("notesRaw"),
    //                 A_DEFFLOAT, A_NULL);

}


void KontrolRack_connect(t_KontrolRack *x, t_floatarg f) {
    if(f>0) {
        std::string host = "127.0.0.1";
        unsigned port = (unsigned) f;
        std::string id = "pd.osc:" + host + ":" + std::to_string(port);
        x->model_->removeCallback(id);
        auto p = std::make_shared<Kontrol::OSCBroadcaster>();
        if (p->connect(host, port)) {
            post("client connected %s", id.c_str());
            x->model_->addCallback(id, p);
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
    if (x->device_) x->device_->encoderButton(0, f);
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

void SendBroadcaster::changed(Kontrol::ParameterSource src,
                              const Kontrol::Rack &rack,
                              const Kontrol::Module &module,
                              const Kontrol::Parameter &param) {

    t_pd *sendobj = get_object(param.id().c_str());
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

void ClientHandler::rack(Kontrol::ParameterSource, const Kontrol::Rack &rack) {
    if (Kontrol::KontrolModel::model()->localRackId() == rack.id()) return;

    std::string id = "pd.osc:" + rack.host() + ":" + std::to_string(rack.port());
    Kontrol::KontrolModel::model()->removeCallback(id);
    if (rack.port()) {
        auto p = std::make_shared<Kontrol::OSCBroadcaster>();
        if (p->connect(rack.host(), rack.port())) {
            Kontrol::KontrolModel::model()->addCallback(id, p);
            post("client handler : connected %s", id.c_str());
        }
    }
}

// puredata methods implementation - end
