#pragma once

#include "m_pd.h"

#include "ParameterModel.h"
#include "Parameter.h"
#include "OSCReceiver.h"
#include "OSCBroadcaster.h"

#include <memory>
#include <unordered_map>

class OrganelleOLED;

struct Knobs {
  enum {
      K_UNLOCKED,
      K_GT,
      K_LT,
      K_LOCKED
  } locked_[4];
};

typedef struct _KontrolHost {
  t_object  x_obj;

  std::shared_ptr<Knobs> knobs_;
  std::shared_ptr<Kontrol::ParameterModel> param_model_;

  std::shared_ptr<OrganelleOLED> oled_;

  unsigned currentPage_;
  long pollCount_;
  std::shared_ptr<Kontrol::OSCReceiver> osc_receiver_;
} t_KontrolHost;


//define pure data methods
extern "C"  {
  t_int*  KontrolHost_tilde_render(t_int *w);
  void    KontrolHost_tilde_dsp(t_KontrolHost *x, t_signal **sp);
  void    KontrolHost_tilde_free(t_KontrolHost*);
  void*   KontrolHost_tilde_new(t_floatarg osc_in);
  void    KontrolHost_tilde_setup(void);

  void    KontrolHost_tilde_listen(t_KontrolHost *x, t_floatarg f);
  void    KontrolHost_tilde_connect(t_KontrolHost *x, t_floatarg f);

  void    KontrolHost_tilde_page(t_KontrolHost *x, t_floatarg f);

  void    KontrolHost_tilde_enc(t_KontrolHost *x, t_floatarg f);
  void    KontrolHost_tilde_encbut(t_KontrolHost *x, t_floatarg f);

  void    KontrolHost_tilde_knob1Raw(t_KontrolHost *x, t_floatarg f);
  void    KontrolHost_tilde_knob2Raw(t_KontrolHost *x, t_floatarg f);
  void    KontrolHost_tilde_knob3Raw(t_KontrolHost *x, t_floatarg f);
  void    KontrolHost_tilde_knob4Raw(t_KontrolHost *x, t_floatarg f);

}
