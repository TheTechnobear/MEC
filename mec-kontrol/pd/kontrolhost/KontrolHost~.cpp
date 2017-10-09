#include "KontrolHost~.h"

#include "OrganelleOLED.h"
#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#include <string>
#include <algorithm>
#include <limits>

/*****
represents the device interface, of which there is always one
******/

static t_class *KontrolHost_tilde_class;


class SendBroadcaster : public  Kontrol::ParameterCallback {
public:
  // ParameterCallback
  virtual void addClient(const std::string& , unsigned ) {;}
  virtual void page(Kontrol::ParameterSource , const Kontrol::Page& ) { ; } // not interested
  virtual void param(Kontrol::ParameterSource, const Kontrol::Parameter&) { ; } // not interested
  virtual void changed(Kontrol::ParameterSource src, const Kontrol::Parameter& p);
};



class ClientHandler : public Kontrol::ParameterCallback {
public:
  ClientHandler(t_KontrolHost* x) : x_(x) {;}
  virtual void addClient(const std::string&, unsigned );
  virtual void page(Kontrol::ParameterSource , const Kontrol::Page& ) { ; } // not interested
  virtual void param(Kontrol::ParameterSource, const Kontrol::Parameter&) { ; } // not interested
  virtual void changed(Kontrol::ParameterSource, const Kontrol::Parameter& );
private:
  t_KontrolHost *x_;
};


// number of dsp renders before poll osc
static const int OSC_POLL_FREQUENCY  = 50;
static const int OLED_POLL_FREQUENCY = 50;


// render count timeout
static const int PAGE_SWITCH_TIMEOUT = 5;
static const int PAGE_EXIT_TIMEOUT = 5;



// puredata methods implementation - start

// helpers

static t_pd *get_object(const char *s) {
  t_pd *x = gensym(s)->s_thing;
  return x;
}


static void sendPdMessage(const char* obj, float f) {
  t_pd* sendobj = get_object(obj);
  if (!sendobj) { post("send to %s failed",obj); return; }

  t_atom a;
  SETFLOAT(&a, f);
  pd_forwardmess(sendobj, 1, &a);
}

static std::string get_param_id(t_KontrolHost *x, unsigned paramnum) {
  auto pageId = x->param_model_->getPageId(x->currentPage_);
  if (pageId.empty()) return "";
  auto page = x->param_model_->getPage(pageId);
  if (page == nullptr) return "";
  auto id = x->param_model_->getParamId(page->id(), paramnum);
  return id;
}


/// main PD methods
t_int* KontrolHost_tilde_render(t_int *w)
{
  t_KontrolHost *x = (t_KontrolHost *)(w[1]);
  x->pollCount_++;
  if (x->osc_receiver_ && x->pollCount_ % OSC_POLL_FREQUENCY == 0) {
    x->osc_receiver_->poll();
  }

  if (x->oled_ && x->pollCount_ % OLED_POLL_FREQUENCY == 0) {
    x->oled_->poll();
  }
  return (w + 2); // # args + 1
}

void KontrolHost_tilde_dsp(t_KontrolHost *x, t_signal **)
{
  // add the perform method, with all signal i/o
  dsp_add(KontrolHost_tilde_render, 1, x);
}

void KontrolHost_tilde_free(t_KontrolHost* x)
{
  x->param_model_->clearCallbacks();
  if (x->osc_receiver_) x->osc_receiver_->stop();
  x->osc_receiver_.reset();
  x->oled_.reset();
  x->knobs_.reset();
  // Kontrol::ParameterModel::free();
}

void *KontrolHost_tilde_new(t_floatarg osc_in)
{
  t_KontrolHost *x = (t_KontrolHost *) pd_new(KontrolHost_tilde_class);

  x->knobs_ = std::make_shared<Knobs>();

  x->osc_receiver_ = nullptr;

  x->pollCount_ = 0;
  x->param_model_ = Kontrol::ParameterModel::model();
  x->oled_ = std::make_shared<OrganelleOLED>(x);
  x->param_model_->addCallback("pd.oled", x->oled_);
  x->param_model_->addCallback("pd.send", std::make_shared<SendBroadcaster>());
  x->param_model_->addCallback("pd.client", std::make_shared<ClientHandler>(x));

  // setup mother.pd for reasonable behaviour, basically takeover
  sendPdMessage("midiOutGate",0.0f);
  // sendPdMessage("midiInGate",0.0f);
  sendPdMessage("enableSubMenu",1.0f);

  KontrolHost_tilde_listen(x, osc_in); // if zero will ignore
  for(int i=0;i<4;i++) {
        x->knobs_->locked_[i]=Knobs::K_UNLOCKED;
  }
  return (void *)x;
}

void KontrolHost_tilde_setup(void) {
  KontrolHost_tilde_class = class_new(gensym("KontrolHost~"),
                                  (t_newmethod) KontrolHost_tilde_new,
                                  (t_method) KontrolHost_tilde_free,
                                  sizeof(t_KontrolHost),
                                  CLASS_DEFAULT,
                                  A_DEFFLOAT, A_NULL);
  class_addmethod(  KontrolHost_tilde_class,
                    (t_method) KontrolHost_tilde_dsp,
                    gensym("dsp"), A_NULL);


  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_listen, gensym("listen"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_connect, gensym("connect"),
                  A_DEFFLOAT, A_NULL);

  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_page, gensym("page"),
                  A_DEFFLOAT, A_NULL);

  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_knob1Raw, gensym("knob1Raw"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_knob2Raw, gensym("knob2Raw"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_knob3Raw, gensym("knob3Raw"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_knob4Raw, gensym("knob4Raw"),
                  A_DEFFLOAT, A_NULL);

  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_enc, gensym("enc"),
                  A_DEFFLOAT, A_NULL);
  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_encbut, gensym("encbut"),
                  A_DEFFLOAT, A_NULL);
  // class_addmethod(KontrolHost_tilde_class,
  //                 (t_method) KontrolHost_tilde_auxRaw, gensym("auxRaw"),
  //                 A_DEFFLOAT, A_NULL);
  // class_addmethod(KontrolHost_tilde_class,
  //                 (t_method) KontrolHost_tilde_vol, gensym("vol"),
  //                 A_DEFFLOAT, A_NULL);
  // class_addmethod(KontrolHost_tilde_class,
  //                 (t_method) KontrolHost_tilde_expRaw, gensym("expRaw"),
  //                 A_DEFFLOAT, A_NULL);
  // class_addmethod(KontrolHost_tilde_class,
  //                 (t_method) KontrolHost_tilde_fsRaw, gensym("fsRaw"),
  //                 A_DEFFLOAT, A_NULL);
  // class_addmethod(KontrolHost_tilde_class,
  //                 (t_method) KontrolHost_tilde_notesRaw, gensym("notesRaw"),
  //                 A_DEFFLOAT, A_NULL);

}



void    KontrolHost_tilde_connect(t_KontrolHost *x, t_floatarg f) {
  std::string host = "127.0.0.1";
  unsigned port = (unsigned) f;
  std::string id = "pd.osc:" + host + ":" + std::to_string(port);
  x->param_model_->removeCallback(id);
  if (port) {
    auto p = std::make_shared<Kontrol::OSCBroadcaster>();
    if (p->connect(host, port)) {
      post("client connected %s" , id.c_str());
      x->param_model_->addCallback(id, p);
    }
  }
}

void    KontrolHost_tilde_listen(t_KontrolHost *x, t_floatarg f) {
  if (f > 0) {
    auto p = std::make_shared<Kontrol::OSCReceiver>(x->param_model_);
    if (p->listen((unsigned) f)) {
      x->osc_receiver_ = p;
    }
  } else {
    x->osc_receiver_.reset();
  }
}

void    KontrolHost_tilde_page(t_KontrolHost *x, t_floatarg f) {
  unsigned page = (unsigned) f;
  page = std::min(page, x->param_model_->getPageCount() - 1);
  x->currentPage_ = page;
  for (int i = 0; i < 4; i++) {
    x->knobs_->locked_[i] = Knobs::K_LOCKED;
  }
}

static void changeEncoder(t_KontrolHost *x, t_floatarg f) {
  unsigned pagenum = x->currentPage_;
  if(f>0) { 
      // clockwise
      pagenum++;
      pagenum = std::min(pagenum, x->param_model_->getPageCount() - 1);
  } else {
      // anti clockwise
      if(pagenum>0) pagenum--;
  }

  if(pagenum!=x->currentPage_) {
      auto pageId = x->param_model_->getPageId(pagenum);
      if (pageId.empty()) return;
      auto page = x->param_model_->getPage(pageId);
      if (page == nullptr) return;
      if (x->oled_) x->oled_->displayPopup(page->displayName(), PAGE_SWITCH_TIMEOUT);

      x->currentPage_ = pagenum;
      for (int i = 0; i < 4; i++) {
        x->knobs_->locked_[i] = Knobs::K_LOCKED;
      }
  }
}

static void encoderButton(t_KontrolHost *x, t_floatarg f) {
  post("encoder button %f", f);
  if (f > 0) {
    if (x->oled_) x->oled_->displayPopup("exit", PAGE_EXIT_TIMEOUT);
    sendPdMessage("goHome",1.0);
  }
}

void    KontrolHost_tilde_enc(t_KontrolHost *x, t_floatarg f) {
  changeEncoder(x, f);
}

void    KontrolHost_tilde_encbut(t_KontrolHost *x, t_floatarg f) {
  encoderButton(x, f);
}

static const unsigned MAX_KNOB_VALUE = 1023;
static void changeKnob(t_KontrolHost *x, t_floatarg f, unsigned knob) {
  auto id = get_param_id(x, knob);
  auto param = x->param_model_->getParam(id);
  if (param == nullptr) return;
  if (!id.empty())  {
    Kontrol::ParamValue calc = param->calcFloat(f / MAX_KNOB_VALUE);
    if (x->knobs_->locked_[knob]!=Knobs::K_UNLOCKED) {
      //if knob is locked, determined if we can unlock it
      if (calc == param->current()) {
        x->knobs_->locked_[knob] = Knobs::K_UNLOCKED;
      }
      else if (x->knobs_->locked_[knob]==Knobs::K_GT) {
        if(calc > param->current()) x->knobs_->locked_[knob] = Knobs::K_UNLOCKED;
      }
      else if (x->knobs_->locked_[knob]==Knobs::K_LT) {
        if(calc < param->current()) x->knobs_->locked_[knob] = Knobs::K_UNLOCKED;
      }
      else if (x->knobs_->locked_[knob]==Knobs::K_LOCKED) {
        // initial locked, determine unlock condition
        if(calc > param->current()) {
            // knob starts greater than param, so wait for it to go less than
            x->knobs_->locked_[knob] = Knobs::K_LT;
        } else {
            // knob starts less than param, so wait for it to go greater than
            x->knobs_->locked_[knob] = Knobs::K_GT;
        }
      }
    }

    if (x->knobs_->locked_[knob]==Knobs::K_UNLOCKED) {
      x->param_model_->changeParam(Kontrol::PS_LOCAL, id, calc);
    }
  }
}


void    KontrolHost_tilde_knob1Raw(t_KontrolHost *x, t_floatarg f) {
  changeKnob(x, f, 0);
}

void    KontrolHost_tilde_knob2Raw(t_KontrolHost *x, t_floatarg f) {
  changeKnob(x, f, 1);
}

void    KontrolHost_tilde_knob3Raw(t_KontrolHost *x, t_floatarg f) {
  changeKnob(x, f, 2);
}

void    KontrolHost_tilde_knob4Raw(t_KontrolHost *x, t_floatarg f) {
  changeKnob(x, f, 3);
}


void SendBroadcaster::changed(Kontrol::ParameterSource, const Kontrol::Parameter& param) {
  t_pd* sendobj = get_object(param.id().c_str());
  if (!sendobj) { post("send to %s failed",param.id().c_str()); return; }

  t_atom a;
  switch (param.current().type()) {
  case Kontrol::ParamValue::T_Float : {
    SETFLOAT(&a, param.current().floatValue());
    break;
  }
  case Kontrol::ParamValue::T_String:
  default:  {
    SETSYMBOL(&a, gensym(param.current().stringValue().c_str()));
    break;
  }
  }
  pd_forwardmess(sendobj, 1, &a);
}



void ClientHandler::addClient(const std::string& host, unsigned port) {
  std::string id = "pd.osc:" + host + ":" + std::to_string(port);
  Kontrol::ParameterModel::model()->removeCallback(id);
  if (port) {
    auto p = std::make_shared<Kontrol::OSCBroadcaster>();
    if (p->connect(host, port)) {
      Kontrol::ParameterModel::model()->addCallback(id, p);
      post("client handler : connected %s" , id.c_str());
    }
  }
}

void ClientHandler::changed(Kontrol::ParameterSource src, const Kontrol::Parameter& param) {
  if (src != Kontrol::PS_LOCAL) {
    auto pageId = Kontrol::ParameterModel::model()->getPageId(x_->currentPage_);
    if (pageId.empty()) return;
    for (int i = 0; i < 4; i++) {
      auto paramid = Kontrol::ParameterModel::model()->getParamId(pageId, i);
      if (paramid.empty()) return;
      if (paramid == param.id()) {
        x_->knobs_->locked_[i] = Knobs::K_LOCKED;
        return;
      }
    }
  }
}


// puredata methods implementation - end

