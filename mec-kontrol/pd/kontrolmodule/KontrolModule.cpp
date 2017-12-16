#include "../m_pd.h"

#include <KontrolModel.h>
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
                Kontrol::KontrolModel::model()->createModule(Kontrol::PS_LOCAL, rackId, moduleId, moduleType,
                                                             moduleType);
            }

            Kontrol::KontrolModel::model()->loadModuleDefinitions(rackId, moduleId, moduleType + "-module.json");
            rack->loadSettings(rackId + "-rack.json");
            rack->applyPreset("Default");
            rack->dumpSettings();
            rack->dumpParameters();
            rack->dumpCurrentValues();
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
}

// puredata methods implementation - end

