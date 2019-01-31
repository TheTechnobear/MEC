#include "KontrolMonitor.h"
#include <KontrolModel.h>

#include "../m_pd.h"

#include <string>
#include <cstring>

static t_class *KontrolMonitor_class;

struct t_KontrolMonitor {
    t_object x_obj;
    t_symbol *symbol;

    char *rack;
    char *module;
    char *param;
};

t_KontrolMonitor * KontrolMonitor_new(t_symbol *symbol, const Kontrol::Rack &rack, const Kontrol::Module &module, const Kontrol::Parameter &param) {
    auto *x = (t_KontrolMonitor*)pd_new(KontrolMonitor_class);

    x->symbol = symbol;
    x->rack = strdup(rack.id().c_str());
    x->module = strdup(module.id().c_str());
    x->param = strdup(param.id().c_str());

    pd_bind(&x->x_obj.ob_pd, x->symbol);

    std::string symbolString = std::string(x->symbol->s_name);

    t_pd *range = gensym((symbolString + "-range").c_str())->s_thing;
    if (range) {
        t_atom args[2];
        SETFLOAT(&args[0], param.calcMinimum().floatValue());
        SETFLOAT(&args[1], param.calcMaximum().floatValue());
        pd_forwardmess(range, 2, args);
    }

    pd_bind(&x->x_obj.ob_pd, gensym((symbolString + "-load").c_str()));

    return x;
}

void KontrolMonitor_free(t_KontrolMonitor *x) {
    std::string loadSymbol = std::string(x->symbol->s_name) + "-load";
    pd_unbind(&x->x_obj.ob_pd, x->symbol);
    pd_unbind(&x->x_obj.ob_pd, gensym(loadSymbol.c_str()));
    free(x->rack);
    free(x->module);
    free(x->param);

    pd_free((t_pd*)&x->x_obj);
}

static void KontrolMonitor_float(t_KontrolMonitor *x, t_floatarg f) {
    Kontrol::KontrolModel::model()->changeParam(Kontrol::CS_LOCAL, x->rack, x->module, x->param, f);
}

static void KontrolMonitor_bang(t_KontrolMonitor *x) {
    auto model = Kontrol::KontrolModel::model();
    auto rack = model->getRack(x->rack);
    auto module = model->getModule(rack, x->module);
    auto param = model->getParam(module, x->param);

    if (!param)
        return;

    float value = param->current().floatValue();
    float min = param->calcMinimum().floatValue();
    float max = param->calcMaximum().floatValue();

    t_pd *sendsym = x->symbol->s_thing;
    if (sendsym) {
        t_atom arg;
        SETFLOAT(&arg, value);
        pd_forwardmess(sendsym, 1, &arg);
    }

    std::string symbolString = x->symbol->s_name;
    t_pd *range = gensym((symbolString + "-range").c_str())->s_thing;
    if (range) {
        t_atom args[2];
        SETFLOAT(&args[0], min);
        SETFLOAT(&args[1], max);
        pd_forwardmess(range, 2, args);
    }

    t_pd *name = gensym((symbolString + "-name").c_str())->s_thing;
    if (name) {
        t_atom arg;

        std::string displayName = param->displayName();
        for (auto &c : displayName) {
            if (c == ' ')
                c = '_';
        }

        SETSYMBOL(&arg, gensym(displayName.c_str()));
        pd_forwardmess(name, 1, &arg);
    }
}

void KontrolMonitor_setup(void) {
    KontrolMonitor_class = class_new(gensym("KontrolMonitor"),
        NULL,
        NULL,
        sizeof(t_KontrolMonitor),
        CLASS_DEFAULT,
        A_NULL);

    class_addfloat(KontrolMonitor_class, (t_method)KontrolMonitor_float);
    class_addbang(KontrolMonitor_class, (t_method)KontrolMonitor_bang);
}
