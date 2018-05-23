#include "../m_pd.h"

#include <KontrolModel.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>

static t_class *KontrolModule_class;

typedef struct _KontrolModule {
    t_object x_obj;
    t_symbol* rackId;
    t_symbol* moduleId;
    t_symbol* moduleType;
    char* modId;
    char* modType;
} t_KontrolModule;


//define pure data methods
extern "C" {
void KontrolModule_free(t_KontrolModule *);
void *KontrolModule_new(t_symbol *name, t_symbol *type);
EXTERN void KontrolModule_setup(void);
void KontrolModule_loaddefinitions(t_KontrolModule *x, t_symbol *defs);
}
// puredata methods implementation - start

void KontrolModule_free(t_KontrolModule *x) {
    free(x->modId);
    free(x->modType);
}

static bool createModule(t_KontrolModule *x) {
    auto rack = Kontrol::KontrolModel::model()->getLocalRack();
    if (rack && x->moduleType && x->moduleId) {
        auto rackId = rack->id();
        Kontrol::KontrolModel::model()->createModule(Kontrol::CS_LOCAL, rackId,
            x->moduleId->s_name, x->moduleType->s_name,
            x->moduleType->s_name);
        rack->dumpParameters();
        x->rackId = gensym(rackId.c_str());
        return true;
    }
    x->rackId = nullptr;

    return false;
}

void *KontrolModule_new(t_symbol *name, t_symbol *type) {
    t_KontrolModule *x = (t_KontrolModule *) pd_new(KontrolModule_class);
    
    if (name && name->s_name && type && type->s_name) {
        
        Kontrol::EntityId moduleId = name->s_name;
        std::string moduleType = type->s_name;
        x->moduleId = gensym(moduleId.c_str());
        x->moduleType = gensym(moduleType.c_str());

        createModule(x);
    } else {
        // missing either module id or type
        x->moduleId = nullptr;
        x->moduleType = nullptr;
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
                    (t_method) KontrolModule_loaddefinitions, gensym("loaddefinitions"),
                    A_DEFSYMBOL, A_NULL);
}


void KontrolModule_loaddefinitions(t_KontrolModule *x, t_symbol *defs) {
    if (defs != nullptr && defs->s_name != nullptr && strlen(defs->s_name) > 0) {
        // if the rack was not available when creating, try again now to create module
        if (x->rackId == nullptr) {
            if (!createModule(x)) {
                post("cannot create %s : No local rack found, KontrolModule needs a KontrolRack instance", x->moduleId->s_name);
                return;
            }
        }
        std::string file = std::string(defs->s_name);
        Kontrol::KontrolModel::model()->loadModuleDefinitions(x->rackId->s_name, x->moduleId->s_name, file);
    }
}




// puredata methods implementation - end

