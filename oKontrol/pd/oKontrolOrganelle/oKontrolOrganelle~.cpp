#include "oKontrolOrganelle~.h"

#include "OrganelleOLED.h"
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#include <string>
#include <algorithm>
#include <limits>

/*****
represents the device interface, of which there is always one
******/

static t_class *oKontrolOrganelle_tilde_class;

static t_pd *get_object(const char *s) {
  t_pd *x = gensym(s)->s_thing;
  return x;
}


class SendBroadcaster : public  oKontrol::ParameterCallback {
public:
  // ParameterCallback
  virtual void addClient(const std::string& , unsigned ) {;}
  virtual void page(oKontrol::ParameterSource , const oKontrol::Page& ) { ; } // not interested
  virtual void param(oKontrol::ParameterSource, const oKontrol::Parameter&) { ; } // not interested
  virtual void changed(oKontrol::ParameterSource src, const oKontrol::Parameter& p);
};



class ClientHandler : public oKontrol::ParameterCallback {
  virtual void addClient(const std::string&, unsigned );
  virtual void page(oKontrol::ParameterSource , const oKontrol::Page& ) { ; } // not interested
  virtual void param(oKontrol::ParameterSource, const oKontrol::Parameter&) { ; } // not interested
  virtual void changed(oKontrol::ParameterSource, const oKontrol::Parameter& ) {;} // not interested
};


// number of dsp renders before poll osc
static const int OSC_POLL_FREQUENCY = 300;


// puredata methods implementation - start

t_int* oKontrolOrganelle_tilde_render(t_int *w)
{
  t_oKontrolOrganelle *x = (t_oKontrolOrganelle *)(w[1]);
  if (x->osc_receiver_) {
    if (++(x->pollCount_) % OSC_POLL_FREQUENCY == 0) {
      x->osc_receiver_->poll();
    }
  }
  return (w + 2); // # args + 1
}

void oKontrolOrganelle_tilde_dsp(t_oKontrolOrganelle *x, t_signal **)
{
  // add the perform method, with all signal i/o
  dsp_add(oKontrolOrganelle_tilde_render, 1, x);
}

void oKontrolOrganelle_tilde_free(t_oKontrolOrganelle* x)
{
  if (x->osc_receiver_) x->osc_receiver_->stop();
  x->osc_receiver_.reset();
  x->param_model_->clearCallbacks();
  // oKontrol::ParameterModel::free();
}

void *oKontrolOrganelle_tilde_new(t_floatarg osc_in)
{
  t_oKontrolOrganelle *x = (t_oKontrolOrganelle *) pd_new(oKontrolOrganelle_tilde_class);

  x->osc_receiver_ = nullptr;

  x->pollCount_ = 0;
  x->param_model_ = oKontrol::ParameterModel::model();

  x->param_model_->addCallback("pd.send", std::make_shared<SendBroadcaster>());
  x->param_model_->addCallback("pd.oled", std::make_shared<OrganelleOLED>(x));
  x->param_model_->addCallback("pd.client", std::make_shared<ClientHandler>());


  oKontrolOrganelle_tilde_listen(x, osc_in); // if zero will ignore
  return (void *)x;
}

void oKontrolOrganelle_tilde_setup(void) {
  oKontrolOrganelle_tilde_class = class_new(gensym("oKontrolOrganelle~"),
                                  (t_newmethod) oKontrolOrganelle_tilde_new,
                                  (t_method) oKontrolOrganelle_tilde_free,
                                  sizeof(t_oKontrolOrganelle),
                                  CLASS_DEFAULT,
                                  A_DEFFLOAT, A_NULL);
  class_addmethod(  oKontrolOrganelle_tilde_class,
                    (t_method) oKontrolOrganelle_tilde_dsp,
                    gensym("dsp"), A_NULL);


  class_addmethod(oKontrolOrganelle_tilde_class,
                  (t_method) oKontrolOrganelle_tilde_listen, gensym("listen"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(oKontrolOrganelle_tilde_class,
                  (t_method) oKontrolOrganelle_tilde_connect, gensym("connect"),
                  A_DEFFLOAT, A_NULL);

  class_addmethod(oKontrolOrganelle_tilde_class,
                  (t_method) oKontrolOrganelle_tilde_page, gensym("page"),
                  A_DEFFLOAT, A_NULL);

  class_addmethod(oKontrolOrganelle_tilde_class,
                  (t_method) oKontrolOrganelle_tilde_knob1Raw, gensym("knob1Raw"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(oKontrolOrganelle_tilde_class,
                  (t_method) oKontrolOrganelle_tilde_knob2Raw, gensym("knob2Raw"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(oKontrolOrganelle_tilde_class,
                  (t_method) oKontrolOrganelle_tilde_knob3Raw, gensym("knob3Raw"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(oKontrolOrganelle_tilde_class,
                  (t_method) oKontrolOrganelle_tilde_knob4Raw, gensym("knob4Raw"),
                  A_DEFFLOAT, A_NULL);

  // class_addmethod(oKontrolOrganelle_tilde_class,
  //                 (t_method) oKontrolOrganelle_tilde_enc, gensym("enc"),
  //                 A_DEFFLOAT, A_NULL);
  // class_addmethod(oKontrolOrganelle_tilde_class,
  //                 (t_method) oKontrolOrganelle_tilde_encbut, gensym("encbut"),
  // A_DEFFLOAT, A_NULL);
  // class_addmethod(oKontrolOrganelle_tilde_class,
  //                 (t_method) oKontrolOrganelle_tilde_auxRaw, gensym("auxRaw"),
  //                 A_DEFFLOAT, A_NULL);
  // class_addmethod(oKontrolOrganelle_tilde_class,
  //                 (t_method) oKontrolOrganelle_tilde_vol, gensym("vol"),
  //                 A_DEFFLOAT, A_NULL);
  // class_addmethod(oKontrolOrganelle_tilde_class,
  //                 (t_method) oKontrolOrganelle_tilde_expRaw, gensym("expRaw"),
  //                 A_DEFFLOAT, A_NULL);
  // class_addmethod(oKontrolOrganelle_tilde_class,
  //                 (t_method) oKontrolOrganelle_tilde_fsRaw, gensym("fsRaw"),
  //                 A_DEFFLOAT, A_NULL);
  // class_addmethod(oKontrolOrganelle_tilde_class,
  //                 (t_method) oKontrolOrganelle_tilde_notesRaw, gensym("notesRaw"),
  //                 A_DEFFLOAT, A_NULL);

}



void    oKontrolOrganelle_tilde_connect(t_oKontrolOrganelle *x, t_floatarg f) {
  std::string host = "127.0.0.1";
  unsigned port = (unsigned) f;
  std::string id = "pd.osc:" + host + ":" + std::to_string(port);
  x->param_model_->removeCallback(id);
  if (port) {
    auto p = std::make_shared<oKontrol::OSCBroadcaster>();
    if (p->connect(host, port)) {
      post("client connected %s" , id.c_str());
      x->param_model_->addCallback(id, p);
    }
  }
}

void    oKontrolOrganelle_tilde_listen(t_oKontrolOrganelle *x, t_floatarg f) {
  if (f > 0) {
    auto p = std::make_shared<oKontrol::OSCReceiver>(x->param_model_);
    if (p->listen((unsigned) f)) {
      x->osc_receiver_ = p;
    }
  } else {
    x->osc_receiver_.reset();
  }
}

void    oKontrolOrganelle_tilde_page(t_oKontrolOrganelle *x, t_floatarg f) {
  unsigned page = (unsigned) f;
  page = std::min(page, x->param_model_->getPageCount() - 1);
  x->currentPage_ = page;
  for (int i = 0; i < 4; i++) {
    x->locked_[i] = true;
  }
}


static std::string get_param_id(t_oKontrolOrganelle *x, unsigned paramnum) {
  auto pageId = x->param_model_->getPageId(x->currentPage_);
  if (pageId.empty()) return "";
  auto page = x->param_model_->getPage(pageId);
  if (page == nullptr) return "";
  auto id = x->param_model_->getParamId(page->id(), paramnum);
  return id;
}

static const unsigned MAX_KNOB_VALUE = 1023;

static void changeKnob(t_oKontrolOrganelle *x, t_floatarg f, unsigned knob) {
  auto id = get_param_id(x, knob);
  auto param = x->param_model_->getParam(id);
  if (param == nullptr) return;
  if (!id.empty())  {
    oKontrol::ParamValue calc = param->calcFloat(f / MAX_KNOB_VALUE);
    if (x->locked_[knob]) {
      //if knob is locked, determined if we can unlock it
      if (calc == param->current()) {
        x->locked_[knob] = false;
      }
      else if (x->knobValue_[knob] > param->current()) {
        x->locked_[knob] = calc > param->current();
      } else {
        x->locked_[knob] = calc < param->current();
      }
    }
    if (!x->locked_[knob]) {
      x->knobValue_[knob] = calc;
      x->param_model_->changeParam(oKontrol::PS_LOCAL, id, calc);
    }
  }
}


void    oKontrolOrganelle_tilde_knob1Raw(t_oKontrolOrganelle *x, t_floatarg f) {
  changeKnob(x, f, 0);
}

void    oKontrolOrganelle_tilde_knob2Raw(t_oKontrolOrganelle *x, t_floatarg f) {
  changeKnob(x, f, 1);
}

void    oKontrolOrganelle_tilde_knob3Raw(t_oKontrolOrganelle *x, t_floatarg f) {
  changeKnob(x, f, 2);
}

void    oKontrolOrganelle_tilde_knob4Raw(t_oKontrolOrganelle *x, t_floatarg f) {
  changeKnob(x, f, 3);
}


void SendBroadcaster::changed(oKontrol::ParameterSource, const oKontrol::Parameter& param) {
  t_pd* sendobj = get_object(param.id().c_str());
  if (!sendobj) { post("no object found to send to"); return; }

  t_atom a;
  switch (param.current().type()) {
  case oKontrol::ParamValue::T_Float : {
    SETFLOAT(&a, param.current().floatValue());
    break;
  }
  case oKontrol::ParamValue::T_String:
  default:  {
    SETSYMBOL(&a, gensym(param.current().stringValue().c_str()));
    break;
  }
  }
  pd_forwardmess(sendobj, 1, &a);
}



void ClientHandler::addClient(const std::string& host, unsigned port) {
  std::string id = "pd.osc:" + host + ":" + std::to_string(port);
  oKontrol::ParameterModel::model()->removeCallback(id);
  if (port) {
    auto p = std::make_shared<oKontrol::OSCBroadcaster>();
    if (p->connect(host, port)) {
      oKontrol::ParameterModel::model()->addCallback(id, p);
      post("client handler : connected %s" , id.c_str());
    }
  }
}


// puredata methods implementation - end

