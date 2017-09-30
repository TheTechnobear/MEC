#pragma once

#include "m_pd.h"

#include "ParameterModel.h"
#include "Parameter.h"
#include "OSCReceiver.h"
#include "OSCBroadcaster.h"

#include <memory>

typedef struct _oKontrolOrganelle {
  t_object  x_obj;

  bool    locked_[4];
  oKontrol::ParamValue   knobValue_[4];

  std::shared_ptr<oKontrol::ParameterModel> param_model_;

  unsigned currentPage_;
  long pollCount_;
  std::shared_ptr<oKontrol::OSCReceiver> osc_receiver_;
  std::shared_ptr<oKontrol::OSCBroadcaster> osc_broadcaster_;
  std::shared_ptr<oKontrol::ParameterCallback> send_broadcaster_;
  std::shared_ptr<oKontrol::ParameterCallback> oled_broadcaster_;
  std::shared_ptr<oKontrol::ParameterCallback> ciient_handler_;
} t_oKontrolOrganelle;


//define pure data methods
extern "C"  {
  t_int*  oKontrolOrganelle_tilde_render(t_int *w);
  void    oKontrolOrganelle_tilde_dsp(t_oKontrolOrganelle *x, t_signal **sp);
  void    oKontrolOrganelle_tilde_free(t_oKontrolOrganelle*);
  void*   oKontrolOrganelle_tilde_new(t_floatarg osc_in);
  void    oKontrolOrganelle_tilde_setup(void);

  void    oKontrolOrganelle_tilde_listen(t_oKontrolOrganelle *x, t_floatarg f);
  void    oKontrolOrganelle_tilde_connect(t_oKontrolOrganelle *x, t_floatarg f);

  void    oKontrolOrganelle_tilde_page(t_oKontrolOrganelle *x, t_floatarg f);

  void    oKontrolOrganelle_tilde_knob1Raw(t_oKontrolOrganelle *x, t_floatarg f);
  void    oKontrolOrganelle_tilde_knob2Raw(t_oKontrolOrganelle *x, t_floatarg f);
  void    oKontrolOrganelle_tilde_knob3Raw(t_oKontrolOrganelle *x, t_floatarg f);
  void    oKontrolOrganelle_tilde_knob4Raw(t_oKontrolOrganelle *x, t_floatarg f);
}