#include "KontrolRack.h"

#ifndef _WIN32

#   include <dirent.h>

#else
#   include "dirent_win.h"
#endif

#include <sys/stat.h>

#include <algorithm>
#include <clocale>


#include "devices/Organelle.h"
#include "devices/MidiControl.h"
#include "devices/Bela.h"

#include "KontrolMonitor.h"

/*****
represents the device interface, of which there is always one
******/

static t_class *KontrolRack_class;

class PdCallback : public Kontrol::KontrolCallback {
public:
    PdCallback(t_KontrolRack *x) : x_(x) {
        ;
    }

    //Kontrol::KontrolCallback
    void rack(Kontrol::ChangeSource, const Kontrol::Rack &) override;

    void module(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override { ; }

    void page(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
              const Kontrol::Page &) override { ; }

    void param(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
               const Kontrol::Parameter &) override;

    void changed(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &,
                 const Kontrol::Parameter &) override;

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &,
                  const std::string &, const std::string &) override { ; }

    void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &) override { ; }

    void loadModule(Kontrol::ChangeSource, const Kontrol::Rack &,
                    const Kontrol::EntityId &, const std::string &) override;

    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;


    void savePreset(Kontrol::ChangeSource, const Kontrol::Rack &, std::string preset) override;
    void loadPreset(Kontrol::ChangeSource, const Kontrol::Rack &, std::string preset) override;
    void saveSettings(Kontrol::ChangeSource, const Kontrol::Rack &) override;

private:
    t_KontrolRack *x_;
};

// puredata methods implementation - start

static const unsigned OSC_POLL_FREQUENCY = 1; // 10ms
static const unsigned DEVICE_POLL_FREQUENCY = 1; // 10ms
static const unsigned OSC_PING_FREQUENCY_SEC = 5;
static const unsigned OSC_PING_FREQUENCY = (OSC_PING_FREQUENCY_SEC * (1000 / 10)); // 5 seconds


// see https://github.com/pure-data/pure-data/blob/master/src/x_time.c
static const float TICK_MS = 1.0f * 10.0f; // 10.ms

// helper methods
void KontrolRack_sendMsg(t_pd *sendObj, const char *msg) {
    if (sendObj == nullptr || msg == nullptr) return;
    t_atom args[1];
    SETSYMBOL(&args[0], gensym(msg));
    pd_forwardmess(sendObj, 1, args);
}

void KontrolRack_sendMsg(t_pd *sendObj, t_symbol *symbol) {
    if (sendObj == nullptr || symbol == nullptr) return;
    t_atom args[1];
    SETSYMBOL(&args[0], symbol);
    pd_forwardmess(sendObj, 1, args);
}

void KontrolRack_obj(t_pd *sendObj, unsigned x, unsigned y, const char *obj, const char *arg) {
    if (sendObj == nullptr || obj == nullptr || arg == nullptr) return;
    t_atom args[5];
    SETSYMBOL(&args[0], gensym("obj"));
    SETFLOAT(&args[1], x);
    SETFLOAT(&args[2], y);
    SETSYMBOL(&args[3], gensym(obj));
    SETSYMBOL(&args[4], gensym(arg));
    pd_forwardmess(sendObj, 5, args);
}

void KontrolRack_connectObjs(t_pd *sendObj, unsigned fromObj, unsigned fromLet, unsigned toObj, unsigned toLet) {
    if (sendObj == nullptr) return;
    t_atom args[5];
    SETSYMBOL(&args[0], gensym("connect"));
    SETFLOAT(&args[1], fromObj);
    SETFLOAT(&args[2], fromLet);
    SETFLOAT(&args[3], toObj);
    SETFLOAT(&args[4], toLet);
    pd_forwardmess(sendObj, 5, args);
}


void KontrolRack_dspState(bool onoff) {
#ifndef __COBALT__ 
    t_pd *pdSendObj = gensym("pd")->s_thing;;
    if (!pdSendObj) {
        post("KontrolRack_dspState: unable to find  pd to change dsp state to %d", onoff);
    } else {
        t_atom args[2];
        SETSYMBOL(&args[0], gensym("dsp"));
        SETFLOAT(&args[1], onoff);
        pd_forwardmess(pdSendObj, 2, args);
    }
#endif
}


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
    x->model_->deleteRack(Kontrol::CS_LOCAL, x->model_->localRackId());
    x->model_->clearCallbacks();
    if (x->osc_receiver_) x->osc_receiver_->stop();
    x->osc_receiver_.reset();
    x->device_.reset();
    // Kontrol::ParameterModel::free();
    for (auto p : *x->param_monitors_) {
        KontrolMonitor_free(p.second);
    }
    delete x->param_monitors_;
}

void *KontrolRack_new(t_symbol* sym, int argc, t_atom *argv) {
    t_KontrolRack *x = (t_KontrolRack *) pd_new(KontrolRack_class);
    x->param_monitors_ = new std::unordered_map<t_symbol *, t_KontrolMonitor*>();

    int clientport = 0;
    int serverport = 0;
    std::string device = "organelle";
    int argcount = 0;

    if(argcount < argc) {
        t_atom* arg = argv + argcount;
        if(arg->a_type == A_FLOAT) {
            // old format: [serverport] [clientport]
            serverport = atom_getfloat(arg);
            argcount++;
            if(argcount < argc) {
                arg = argv + argcount;
                clientport = atom_getfloat(arg);
            }
        } else if (arg->a_type == A_SYMBOL) {
            // new format: [device] [serverport] [clientport]
            t_symbol* sym = atom_getsymbol(arg);
            device = sym->s_name;
            argcount++;
            if(argcount < argc) {
                arg = argv + argcount;
                serverport = atom_getfloat(arg);
                argcount++;
                if(argcount < argc) {
                    arg = argv + argcount;
                    clientport = atom_getfloat(arg);
                }
            }
        }
    }

    x->active_module_ = nullptr;
    x->osc_receiver_ = nullptr;
    x->single_module_mode_ = false;
    x->monitor_enable_ = false;

    x->pollCount_ = 0;
    x->model_ = Kontrol::KontrolModel::model();

    x->model_->createLocalRack((unsigned int) clientport);
    x->model_->localRack()->initPrefs();

    if(device=="organelle") {
        post("KontrolRack: device = %s", device.c_str());
        x->device_ = std::make_shared<Organelle>();
    } else if (device == "bela") {
        post("KontrolRack: device = %s", device.c_str());
        x->device_ = std::make_shared<Bela>();
    } else if (device == "midi") {
        post("KontrolRack: device = %s", device.c_str());
        x->device_ = std::make_shared<MidiControl>();
    } else if (device == "kontrol") {
        post("KontrolRack: device = %s", device.c_str());
        x->device_ = std::make_shared<KontrolDevice>();
    } else if (device == "none") {
        post("KontrolRack: not using a device");
    } else {
        post("KontrolRack: unknown device = %s", device.c_str());
    }

    if(x->device_) {
        x->device_->init();
        x->device_->currentRack(x->model_->localRackId());
        x->model_->addCallback("pd.dev", x->device_);
    }

    x->model_->addCallback("pd.send", std::make_shared<PdCallback>(x));

    KontrolRack_listen(x, clientport);
    KontrolRack_connect(x, serverport);

    x->x_clock = clock_new(x, (t_method) KontrolRack_tick);
    clock_setunit(x->x_clock, TICK_MS, 0);
    clock_delay(x->x_clock, 1);

    KontrolRack_loadresources(x);

    return (void *) x;
}

EXTERN void KontrolRack_setup(void) {
    KontrolRack_class = class_new(gensym("KontrolRack"),
                                  (t_newmethod) KontrolRack_new,
                                  (t_method) KontrolRack_free,
                                  sizeof(t_KontrolRack),
                                  CLASS_DEFAULT,
                                  A_GIMME,
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
                    (t_method) KontrolRack_key, gensym("key"),
                    A_DEFFLOAT, A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                 (t_method) KontrolRack_pgm, gensym("pgm"),
                 A_DEFFLOAT, A_NULL);


    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_midiCC, gensym("midiCC"),
                    A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_modulate, gensym("modulate"),
                    A_DEFSYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_loadsettings, gensym("loadsettings"),
                    A_DEFSYMBOL, A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_savesettings, gensym("savesettings"),
                    A_DEFSYMBOL, A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_loadpreset, gensym("loadpreset"),
                    A_DEFSYMBOL, A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_savepreset, gensym("savepreset"),
                    A_DEFSYMBOL, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_nextpreset, gensym("nextpreset"),
                     A_NULL);
    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_prevpreset, gensym("prevpreset"),
                     A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_savecurrentpreset, gensym("savecurrentpreset"),
                    A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_loadmodule, gensym("loadmodule"),
                    A_DEFSYMBOL, A_DEFSYMBOL, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_singlemodulemode, gensym("singlemodulemode"),
                    A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_monitorenable, gensym("monitorenable"),
                    A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_getparam, gensym("getparam"),
                    A_DEFSYMBOL, A_DEFSYMBOL, A_DEFFLOAT, A_DEFSYMBOL, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_setparam, gensym("setparam"),
                    A_DEFSYMBOL, A_DEFSYMBOL, A_DEFFLOAT, A_NULL);


    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_selectmodule, gensym("selectmodule"),
                    A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_selectpage, gensym("selectpage"),
                    A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_getsetting, gensym("getsetting"),
                    A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_enablemenu, gensym("enablemenu"),
                    A_DEFFLOAT, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_setmoduleorder, gensym("setmoduleorder"),
                    A_GIMME, A_NULL);

    class_addmethod(KontrolRack_class,
                    (t_method) KontrolRack_test, gensym("test"),
                    A_DEFFLOAT, A_NULL);

    KontrolMonitor_setup();
}


void KontrolRack_test(t_KontrolRack *x, t_floatarg f) {
    post("test");
}

// Called from OS specific library unload methods.
void KontrolRack_cleanup(void)
{
    auto model = Kontrol::KontrolModel::model();
    if (model)
    {
        // Delete the local rack on exiting, if there is one.
        auto localRackId = model->localRackId();
        if (!localRackId.empty())
        {
            model->deleteRack(Kontrol::CS_LOCAL, localRackId);
            model->clearCallbacks(); // Triggers stop() and in turn flushes the deleteRack message.
        }
    }
}

void KontrolRack_loadmodule(t_KontrolRack *x, t_symbol *modId, t_symbol *modType) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (rack == nullptr) return;

    if (modId == nullptr && modId->s_name == nullptr && strlen(modId->s_name) == 0) {
        post("loadmodule failed , invalid moduleId");
        return;
    }
    if (modType == nullptr && modType->s_name == nullptr && strlen(modType->s_name) == 0) {
        post("loadmodule failed , invalid module for moduleId %s", modId->s_name);
        return;
    }

    std::string mType = modType->s_name;



    // load the module
    std::string module_dir = rack->moduleDir();
    struct stat st;
    if (rack->userModuleDir().length() > 0) {
        std::string module = rack->userModuleDir() + "/" + mType + "/module.pd";
        int fs = stat(module.c_str(), &st);
        if(fs == 0) {
            module_dir = rack->userModuleDir();
            post ("loading user module %s", mType.c_str());
        }
    }

    std::string moddir = module_dir + "/" + mType;
    t_symbol* modDirSym=gensym(moddir.c_str());

    std::string pdObject = std::string("pd-") + std::string(modId->s_name);
    std::string pdModule = moddir + "/module";

    t_pd *sendObj = gensym(pdObject.c_str())->s_thing;
    if (!sendObj) {
        post("loadmodule: unable to find %s", pdObject.c_str());
        return;
    }

    post("loadmodule: loading %s into %s", modType->s_name, modId->s_name);

    KontrolRack_dspState(false);
    KontrolRack_sendMsg(sendObj, "clear");

    KontrolRack_obj(sendObj, 10, 10, "r~", (std::string("inL-") + modId->s_name).c_str()); //obj0
    KontrolRack_obj(sendObj, 110, 10, "r~", (std::string("inR-") + modId->s_name).c_str()); //obj1
    KontrolRack_obj(sendObj, 10, 60, pdModule.c_str(), modId->s_name); // obj2
    KontrolRack_obj(sendObj, 10, 110, "s~", (std::string("outL-") + modId->s_name).c_str()); // obj3
    KontrolRack_obj(sendObj, 110, 110, "s~", (std::string("outR-") + modId->s_name).c_str()); // obj4

    KontrolRack_connectObjs(sendObj, 0, 0, 2, 0);
    KontrolRack_connectObjs(sendObj, 1, 0, 2, 1);
    KontrolRack_connectObjs(sendObj, 2, 0, 3, 0);
    KontrolRack_connectObjs(sendObj, 2, 1, 4, 0);


    auto module = Kontrol::KontrolModel::model()->getModule(rack, modId->s_name);
    if (module == nullptr) {
        post("unable to initialise module %s %s", modId->s_name, modType->s_name);
        KontrolRack_dspState(true);
        return;
    }


    KontrolRack_obj(sendObj, 400, 1, "r", "rackloaded"); //obj5

    {
        t_atom args[6];
        SETSYMBOL(&args[0], gensym("obj")); //obj 6
        SETFLOAT(&args[1], 400);
        SETFLOAT(&args[2], 30);
        SETSYMBOL(&args[3], gensym("t"));
        SETSYMBOL(&args[4], gensym("b"));
        SETSYMBOL(&args[5], gensym("b"));
        pd_forwardmess(sendObj, 6, args);

    }
    KontrolRack_connectObjs(sendObj, 5, 0, 6, 0);


    {
        // loaddefinitions message
        std::string file = moddir + "/module.json";
        t_symbol *fileSym = gensym(file.c_str());
        t_atom args[5];
        SETSYMBOL(&args[0], gensym("msg"));
        SETFLOAT(&args[1], 400);
        SETFLOAT(&args[2], 60);
        SETSYMBOL(&args[3], gensym("loaddefinitions"));
        SETSYMBOL(&args[4], fileSym);
        pd_forwardmess(sendObj, 5, args); //obj7
    }
    KontrolRack_connectObjs(sendObj, 6, 1, 7, 0);

    {
        t_atom args[6];
        SETSYMBOL(&args[0], gensym("obj"));
        SETFLOAT(&args[1], 400);
        SETFLOAT(&args[2], 110);
        SETSYMBOL(&args[3], gensym("KontrolModule"));
        SETSYMBOL(&args[4], gensym(modId->s_name));
        SETSYMBOL(&args[5], gensym(modType->s_name));
        pd_forwardmess(sendObj, 6, args);
    }

    KontrolRack_connectObjs(sendObj, 7, 0, 8, 0);
    {
        std::string sym = std::string("loadbang-") + modId->s_name;
        KontrolRack_obj(sendObj, 600, 110, "send", sym.c_str()); //obj9
    }

    KontrolRack_connectObjs(sendObj, 6, 0, 9, 0);

    // now initialise the module
    {
        // load definition
        std::string file = moddir + "/module.json";
        Kontrol::KontrolModel::model()->loadModuleDefinitions(rack->id(), module->id(),file);
    }

    {
        // clear aux label and led
        t_pd *sendobj;
        std::string sendsym;
        sendsym = std::string("aux-label-") + modId->s_name;
        sendobj = gensym(sendsym.c_str())->s_thing;
        if (sendobj != nullptr) {
            KontrolRack_sendMsg(sendobj, "   ");
        }

        sendsym = std::string("aux-led-") + modId->s_name;
        sendobj = gensym(sendsym.c_str())->s_thing;
        if (sendobj != nullptr) {
            t_atom args[1];
            SETFLOAT(&args[0], 0);
            pd_forwardmess(sendobj, 1, args);
        }
    }

    {
        // send a fake loadbang after its loaded
        std::string sendsym = std::string("loadbang-") + modId->s_name;
        t_pd *sendobj = gensym(sendsym.c_str())->s_thing;
        if (sendobj != nullptr) {
            pd_bang(sendobj);
        } else {
            post("loadbang missing for %s", sendsym.c_str());
        }
    }


    {
        // send a loadbang global after its loaded
        // this lets other modules know a new module has 'entered the system'
        std::string sendsym = std::string("loadbang-global");
        t_pd *sendobj = gensym(sendsym.c_str())->s_thing;
        if (sendobj != nullptr) {
            pd_bang(sendobj);
        } else {
            post("loadbang missing for %s", sendsym.c_str());
        }
    }



    if (x->active_module_ != nullptr) {
        t_pd *sendobj = gensym("activeModule")->s_thing;
        if (sendobj != nullptr) {
            KontrolRack_sendMsg(sendobj, x->active_module_);
        }
    }

    KontrolRack_dspState(true);
}

void KontrolRack_singlemodulemode(t_KontrolRack *x, t_floatarg f) {
    bool enable = f >= 0.5f;
    post("KontrolRack: single module mode -> %d", enable);
    x->single_module_mode_ = enable;
}

void KontrolRack_monitorenable(t_KontrolRack *x, t_floatarg f) {
    bool enable = f >= 0.5f;
    post("KontrolRack: Enable control monitoring: %d", enable);
    x->monitor_enable_ = enable;
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

void KontrolRack_midiCC(t_KontrolRack *x, t_floatarg ch, t_floatarg cc, t_floatarg value) {
    if (x->device_) x->device_->midiCC(unsigned (ch*127 + cc) , (unsigned) value);
}

void KontrolRack_key(t_KontrolRack *x, t_floatarg key, t_floatarg value) {
    if (x->device_) x->device_->keyPress((unsigned) key, (unsigned) value);
}

void KontrolRack_pgm(t_KontrolRack *x, t_floatarg f) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (!rack) { post("No local rack found"); return; }
    auto presets = rack->getPresetList();
    unsigned pnum = f;
    if( pnum < presets.size() ) {
        auto preset = presets[pnum];
        rack->loadPreset(preset);
        post("pgm change : preset changed to %d : %s", pnum, preset.c_str());
    } else {
        post("pgm change : preset out of range %d", pnum);
    }
}



void KontrolRack_nextpreset(t_KontrolRack *x) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (!rack) { post("No local rack found"); return; }
    auto presets = rack->getPresetList();
    if(presets.size()==0) return;

    auto  curPreset = rack->currentPreset();
    unsigned idx=0, pnum = 0;
    for(auto p : presets) {
        if(p==curPreset) {
            pnum = idx + 1;
            if(pnum > (presets.size()-1)) pnum = 0;
            break;
        }
        idx++;
    }

    if( pnum < presets.size() ) {
        auto preset = presets[pnum];
        rack->loadPreset(preset);
        post("nextpreset : preset changed to %d : %s", pnum, preset.c_str());
    }
}


void KontrolRack_prevpreset(t_KontrolRack *x) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (!rack) { post("No local rack found"); return; }
    auto presets = rack->getPresetList();
    if(presets.size()==0) return;

    auto  curPreset = rack->currentPreset();
    unsigned idx=0, pnum=0;
    for(auto p : presets) {
        if(p==curPreset) {
            if(idx==0) pnum = presets.size() -1;
            else pnum = idx - 1;

            break;
        }
        idx++;
    }

    if( pnum < presets.size() ) {
        auto preset = presets[pnum];
        rack->loadPreset(preset);
        post("prevpreset : preset changed to %d : %s", pnum, preset.c_str());
    }
}


void KontrolRack_savecurrentpreset(t_KontrolRack *x) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (!rack) {
        post("No local rack found");
        return;
    }
    auto presets = rack->getPresetList();
    if (presets.size() == 0) return;

    auto curPreset = rack->currentPreset();
    if (!curPreset.empty()) {
        rack->savePreset(curPreset);
    }
}



void KontrolRack_loadsettings(t_KontrolRack *x, t_symbol *settings) {
    if (settings != nullptr && settings->s_name != nullptr && strlen(settings->s_name) > 0) {
        auto rack = Kontrol::KontrolModel::model()->getLocalRack();
        if (rack) {
            std::string settingsId = settings->s_name;
            post("loading settings : %s", settings->s_name);
            rack->loadSettings(settingsId + ".json");
            rack->dumpSettings();
        } else {
            post("No local rack found");
        }
    }
}

void KontrolRack_savesettings(t_KontrolRack *x, t_symbol *settings) {
    if (settings != nullptr && settings->s_name != nullptr && strlen(settings->s_name) > 0) {
        auto rack = Kontrol::KontrolModel::model()->getLocalRack();
        if (rack) {
            std::string settingsId = settings->s_name;
            post("saving settings : %s", settings->s_name);
            rack->saveSettings(settingsId + ".json");
        } else {
            post("No local rack found");
        }
    } else {
        post("error savesettings name");
    }
}


void KontrolRack_loadpreset(t_KontrolRack *x, t_symbol *preset) {
    if (preset != nullptr && preset->s_name != nullptr && strlen(preset->s_name) > 0) {
        auto rack = Kontrol::KontrolModel::model()->getLocalRack();
        if (rack) {
            std::string presetId = preset->s_name;
            post("loading preset : %s", preset->s_name);
            rack->loadPreset(presetId);
            rack->dumpCurrentValues();
        } else {
            post("No local rack found");
        }
    } else {
        post("error loadpreset name");
    }
}

void KontrolRack_savepreset(t_KontrolRack *x, t_symbol *preset) {
    if (preset != nullptr && preset->s_name != nullptr && strlen(preset->s_name) > 0) {
        auto rack = Kontrol::KontrolModel::model()->getLocalRack();
        if (rack) {
            std::string presetId = preset->s_name;
            post("save preset : %s", preset->s_name);
            rack->savePreset(presetId);
        }
        else {
            post("No local rack found");
        }
    } else {
        post("error save name");
    }
}

void KontrolRack_modulate(t_KontrolRack *x, t_symbol* src, t_floatarg bus, t_floatarg value) {
    if(src== nullptr || src->s_name==nullptr) {
        post("error modulate srcid bus value");
        return;
    }

    if (x->device_) {
        x->device_->modulate(src->s_name, (unsigned) bus, value);
    }
}


void KontrolRack_setparam(t_KontrolRack* x, t_symbol* modId, t_symbol* paramId, t_floatarg value) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (!rack) { post("No local rack found"); return;}

    if(modId != nullptr && modId->s_name != nullptr
       && paramId != nullptr && paramId->s_name != nullptr
       ) {
        auto module = rack->getModule(modId->s_name);
        if (module) {
            auto param = module->getParam(paramId->s_name);
            if (param
                ) {
                Kontrol::KontrolModel::model()->changeParam(
                        Kontrol::CS_LOCAL,
                        rack->id(),
                        module->id(),
                        param->id(),
                        param->calcFloat(value)
                        );
            }
        }

    } else {
        post("error setparam modid paramid value");
    }
}


void KontrolRack_getparam(t_KontrolRack* x,
        t_symbol* modId, t_symbol* paramId,
        t_floatarg defvalue,
        t_symbol* sendsym) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (!rack) { post("No local rack found"); return;}

    if(    sendsym != nullptr && sendsym->s_name != nullptr
        && modId != nullptr && modId->s_name != nullptr
        && paramId != nullptr && paramId->s_name != nullptr
        ) {
        float value = defvalue;

        auto module = rack->getModule(modId->s_name);
        if (module) {
            auto param = module->getParam(paramId->s_name);
            if (param) {
                value = param->asFloat(param->current());
            }
        }

        std::string pdObject=sendsym->s_name;
        t_pd *sendObj = gensym(pdObject.c_str())->s_thing;
//        t_pd* sendObj = gensym(sendsym->s_name)->s_thing;
        if (sendObj != nullptr) {
            t_atom args[1];
            SETFLOAT(&args[0], value);
            pd_forwardmess(sendObj, 1, args);
        } else {
            post("getparam sendsym not found %s", sendsym->s_name);
        }
    } else {
        post("error getparam modid paramid defvalue sendsym");
    }

}


void KontrolRack_getsetting(t_KontrolRack* x,
                          t_symbol* settingName,
                          t_symbol* defvalue,
                          t_symbol* sendsym) {
    if(sendsym != nullptr && sendsym->s_name != nullptr) {
        t_symbol* value = defvalue;

        if (settingName != nullptr && settingName->s_name != nullptr
                ) {
            auto rack = Kontrol::KontrolModel::model()->getLocalRack();
            if (rack) {
                std::string setting = settingName->s_name;
                if(setting == "mediaDir") {
                    value = gensym(rack->mediaDir().c_str());
                } else if (setting == "dataDir") {
                    value = gensym(rack->dataDir().c_str());
                } else if (setting == "currentPreset") {
                    value = gensym(rack->currentPreset().c_str());
                } // else ignore, value is already set to defvalue
            }
        }

        std::string pdObject=sendsym->s_name;
        t_pd *sendObj = gensym(pdObject.c_str())->s_thing;
//        t_pd* sendObj = gensym(sendsym->s_name)->s_thing;
        if (sendObj != nullptr) {
            t_atom args[1];
            SETSYMBOL(&args[0], value);
            pd_forwardmess(sendObj, 1, args);
        } else {
            post("getsetting sendsym not found %s", sendsym->s_name);
        }
    }
}


void loadModuleDir(const std::string& baseDir, const std::string& subDir) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();

    struct stat st;
    struct dirent **namelist;
    std::string dir = baseDir;
    if(subDir.length()>0) dir = baseDir + "/" + subDir;

    int n = scandir(dir.c_str(), &namelist, NULL, alphasort);
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            if (namelist[i]->d_type == DT_DIR &&
                strcmp(namelist[i]->d_name, "..") != 0
                && strcmp(namelist[i]->d_name, ".") != 0) {

                std::string mname = std::string(namelist[i]->d_name);
                if(subDir.length()>0) mname = subDir + "/" + mname;

                std::string module = baseDir + "/" + mname + "/module.pd";
                int fs = stat(module.c_str(), &st);
                if (fs == 0) {
                    rack->addResource("module", mname);
                    post("KontrolRack::module found: %s", mname.c_str());
                } else {
                    loadModuleDir(baseDir, mname);
                }
            }
            free(namelist[i]);
        }
        free(namelist);
    }

}


void KontrolRack_loadresources(t_KontrolRack *x) {
    post("KontrolRack::loading resources");

    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (rack == nullptr) return;

    //load available modules
    struct dirent **namelist;
    struct stat st;

    std::setlocale(LC_ALL, "en_US.UTF-8");

    std::string moddir;
    if(rack->moduleDir().at(0)=='/') moddir = rack->moduleDir();
    else moddir = rack->mainDir()+"/"+rack->moduleDir();

    loadModuleDir(moddir,"");
    if (rack->userModuleDir().length() > 0) {
        loadModuleDir(rack->userModuleDir(),"");
    }
}

void KontrolRack_enablemenu(t_KontrolRack *x, t_floatarg f) {
    bool enable = f >= 0.5f;
    post("KontrolRack: enablemenu -> %d", enable);
    if (x->device_) x->device_->enableMenu(enable);
}



static std::string getParamSymbol(bool singleModuleMode, const Kontrol::Module &module, const Kontrol::Parameter &param) {
    if (!singleModuleMode) {
        return param.id() + "-" + module.id();
    } else {
        return param.id();
    }
}

void KontrolRack_setmoduleorder(t_KontrolRack *x,t_symbol* s, int argc, t_atom *argv) {
    std::string buf;
    for(int i=0; i<argc;i++) {
        t_atom* arg = argv + i;
        if (arg->a_type == A_SYMBOL) {
            t_symbol *sym = atom_getsymbol(arg);
            buf += sym->s_name;
            if(i != argc-1) buf += " ";
        }
    }
    post("module order set to [%s]",buf.c_str());
    Kontrol::KontrolModel::model()->getLocalRack()->addResource("moduleorder",buf);
    Kontrol::KontrolModel::model()->publishResource(Kontrol::CS_LOCAL,
            *Kontrol::KontrolModel::model()->getLocalRack(),
            "moduleorder",buf);
}


void KontrolRack_selectpage(t_KontrolRack* x, t_floatarg page) {
    if (x->device_) x->device_->selectPage((unsigned) page);
}

void KontrolRack_selectmodule(t_KontrolRack* x, t_floatarg module) {
    if (x->device_) x->device_->selectModule((unsigned) module);
}


//-----------------------
void PdCallback::rack(Kontrol::ChangeSource src, const Kontrol::Rack & rack)  {
    auto prack = Kontrol::KontrolModel::model()->getLocalRack();
    if (prack == nullptr || prack->id() != rack.id()) return;

    KontrolRack_sendMsg(gensym("rackCurrentPreset")->s_thing, gensym(prack->currentPreset().c_str()));
    KontrolRack_sendMsg(gensym("rackDataDir")->s_thing, gensym(prack->dataDir().c_str()));
    KontrolRack_sendMsg(gensym("rackMediaDir")->s_thing, gensym(prack->mediaDir().c_str()));
}

void PdCallback::changed(Kontrol::ChangeSource src,
                         const Kontrol::Rack &rack,
                         const Kontrol::Module &module,
                         const Kontrol::Parameter &param) {

    auto prack = Kontrol::KontrolModel::model()->getLocalRack();
    if (prack == nullptr || prack->id() != rack.id()) return;

    std::string sendsym = getParamSymbol(x_->single_module_mode_, module, param);

    t_pd *sendobj = gensym(sendsym.c_str())->s_thing;


    if (!sendobj) {
        post("send to %s failed", param.id().c_str());
        return;
    }

    t_atom a;
    switch (param.current().type()) {
        case Kontrol::ParamValue::T_Float : {
//            post("changed : %s %f", sendsym.c_str(),param.current().floatValue());
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

void PdCallback::param(Kontrol::ChangeSource src,
                       const Kontrol::Rack &rack,
                       const Kontrol::Module &module,
                       const Kontrol::Parameter &param) {
    PdCallback::changed(src, rack, module, param);

    t_symbol *symbol = gensym(getParamSymbol(x_->single_module_mode_, module, param).c_str());

    if (x_->monitor_enable_ && x_->param_monitors_->find(symbol) == x_->param_monitors_->end()) {
        t_KontrolMonitor *x = (t_KontrolMonitor*)KontrolMonitor_new(symbol, rack, module, param);

        (*x_->param_monitors_)[symbol] = x;
    }
}

void PdCallback::loadModule(Kontrol::ChangeSource, const Kontrol::Rack &r,
                            const Kontrol::EntityId &mId, const std::string &mType) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (rack && rack->id() == r.id()) {
        t_symbol *modId = gensym(mId.c_str());
        t_symbol *modType = gensym(mType.c_str());
        KontrolRack_loadmodule(x_, modId, modType);
    } else {
        post("No local rack found");
    }
}

void PdCallback::activeModule(Kontrol::ChangeSource, const Kontrol::Rack &r, const Kontrol::Module &m) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (rack && rack->id() == r.id()) {
        t_pd *sendobj;
        sendobj = gensym("screenLine5")->s_thing;
        if (sendobj != nullptr) {
            KontrolRack_sendMsg(sendobj, "   ");
        }

        sendobj = gensym("led")->s_thing;
        if (sendobj != nullptr) {
            t_atom args[1];
            SETFLOAT(&args[0], 0);
            pd_forwardmess(sendobj, 1, args);
        }

        x_->active_module_ = gensym(m.id().c_str());
        sendobj = gensym("activeModule")->s_thing;
        if (sendobj != nullptr) {
            KontrolRack_sendMsg(sendobj, x_->active_module_);
        }
//        post("active module changed to : %s", m.id().c_str());
    }
}

void PdCallback::savePreset(Kontrol::ChangeSource, const Kontrol::Rack &r , std::string preset) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (rack && rack->id() != r.id())return;
    post("preset saved : %s", preset.c_str());
    KontrolRack_sendMsg(gensym("rackSavePreset")->s_thing, gensym(preset.c_str()));
}

void PdCallback::loadPreset(Kontrol::ChangeSource, const Kontrol::Rack & r, std::string preset) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (rack && rack->id() != r.id())return;
    post("preset loaded  : %s", preset.c_str());
    KontrolRack_sendMsg(gensym("rackLoadPreset")->s_thing, gensym(preset.c_str()));
    KontrolRack_sendMsg(gensym("rackCurrentPreset")->s_thing, gensym(rack->currentPreset().c_str()));
}

void PdCallback::saveSettings(Kontrol::ChangeSource, const Kontrol::Rack & r ) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (rack && rack->id() != r.id())return;
    post("rack settings saved");
    KontrolRack_sendMsg(gensym("rackSaveSettings")->s_thing, gensym(r.id().c_str()));
}

// puredata methods implementation - end
