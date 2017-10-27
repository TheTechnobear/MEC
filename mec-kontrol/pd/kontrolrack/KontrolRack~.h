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
  t_object  x_obj;

  std::shared_ptr<Kontrol::KontrolModel> model_;
  std::shared_ptr<Organelle> device_;

  long pollCount_;
  std::shared_ptr<Kontrol::OSCReceiver> osc_receiver_;
} t_KontrolRack;


//define pure data methods
extern "C"  {
  t_int*  KontrolRack_tilde_render(t_int *w);
  void    KontrolRack_tilde_dsp(t_KontrolRack *x, t_signal **sp);
  void    KontrolRack_tilde_free(t_KontrolRack*);
  void*   KontrolRack_tilde_new(t_floatarg osc_in);
  void    KontrolRack_tilde_setup(void);

  void    KontrolRack_tilde_listen(t_KontrolRack *x, t_floatarg f);
  void    KontrolRack_tilde_connect(t_KontrolRack *x, t_floatarg f);

  void    KontrolRack_tilde_enc(t_KontrolRack *x, t_floatarg f);
  void    KontrolRack_tilde_encbut(t_KontrolRack *x, t_floatarg f);
  void    KontrolRack_tilde_knob1Raw(t_KontrolRack *x, t_floatarg f);
  void    KontrolRack_tilde_knob2Raw(t_KontrolRack *x, t_floatarg f);
  void    KontrolRack_tilde_knob3Raw(t_KontrolRack *x, t_floatarg f);
  void    KontrolRack_tilde_knob4Raw(t_KontrolRack *x, t_floatarg f);
  void    KontrolRack_tilde_midiCC(t_KontrolRack *x, t_floatarg cc, t_floatarg value);
}
