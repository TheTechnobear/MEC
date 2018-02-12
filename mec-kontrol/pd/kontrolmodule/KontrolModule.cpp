#include "../m_pd.h"

#include <KontrolModel.h>
#include <string.h>
#include <string>
#include <vector>

static t_class *KontrolModule_class;

typedef struct _KontrolModule {
    t_object x_obj;
} t_KontrolModule;


//define pure data methods
extern "C" {
void KontrolModule_free(t_KontrolModule *);
void *KontrolModule_new(t_symbol *name, t_symbol *type);
void KontrolModule_setup(void);

void KontrolModule_loadsettings(t_KontrolModule *x, t_symbol *settings);
void KontrolModule_loadpreset(t_KontrolModule *x, t_symbol *preset);
}
// puredata methods implementation - start

void KontrolModule_free(t_KontrolModule *x) {
}

void *KontrolModule_new(t_symbol *name, t_symbol *type) {
    t_KontrolModule *x = (t_KontrolModule *) pd_new(KontrolModule_class);
    if (name && name->s_name && type && type->s_name) {
        auto rack = Kontrol::KontrolModel::model()->getLocalRack();
        if (rack) {
            auto rackId = rack->id();
            Kontrol::EntityId moduleId = name->s_name;
            std::string moduleType = type->s_name;
            if (rack->getModule(moduleId) == nullptr) {
                Kontrol::KontrolModel::model()->createModule(Kontrol::CS_LOCAL, rackId, moduleId, moduleType,
                                                             moduleType);
                rack->dumpParameters();
            }
            Kontrol::KontrolModel::model()->loadModuleDefinitions(rackId, moduleId, moduleType + "-module.json");
        } else {
            post("No local rack found, KontrolModule needs a KontrolRack instance");
        }
    }
    return (void *) x;
}

void KontrolModule_setup(void) {
    KontrolModule_class = class_new(gensym("KontrolModule"),
                                    (t_newmethod) KontrolModule_new,
                                    (t_method) KontrolModule_free,
                                    sizeof(t_KontrolModule),
                                    CLASS_DEFAULT,
                                    A_SYMBOL, A_SYMBOL, A_NULL);
    class_addmethod(KontrolModule_class,
                    (t_method) KontrolModule_loadsettings, gensym("loadsettings"),
                    A_DEFSYMBOL, A_NULL);
    class_addmethod(KontrolModule_class,
                    (t_method) KontrolModule_loadpreset, gensym("loadpreset"),
                    A_DEFSYMBOL, A_NULL);
}


void KontrolModule_loadsettings(t_KontrolModule *x, t_symbol *settings) {
    if(settings!= nullptr && settings->s_name != nullptr && strlen(settings->s_name)>0) {
        auto rack = Kontrol::KontrolModel::model()->getLocalRack();
        if (rack) {
            std::string settingsId = settings->s_name;
            post("loading settings : %s", settings->s_name);
            rack->loadSettings(settingsId + "-rack.json");
            rack->dumpSettings();
        } else {
            post("No local rack found, KontrolModule needs a KontrolRack instance");
        };
    }
}

void KontrolModule_loadpreset(t_KontrolModule *x, t_symbol *preset) {
    if(preset!= nullptr && preset->s_name != nullptr && strlen(preset->s_name)>0) {
        auto rack = Kontrol::KontrolModel::model()->getLocalRack();
        if (rack) {
            std::string presetId = preset->s_name;
            post("loading preset : %s", preset->s_name);
            rack->applyPreset(presetId);
            rack->dumpCurrentValues();
        }
    } else {
        post("No local rack found, KontrolModule needs a KontrolRack instance");
    }
}


// puredata methods implementation - end

