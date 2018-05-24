#pragma once

#include "../m_pd.h"

struct t_KontrolMonitor;

namespace Kontrol {
    class Rack;
    class Module;
    class Parameter;
}

t_KontrolMonitor * KontrolMonitor_new(t_symbol *symbol, const Kontrol::Rack &rack, const Kontrol::Module &module, const Kontrol::Parameter &param);
void KontrolMonitor_free(t_KontrolMonitor *obj);
void KontrolMonitor_setup(void);
