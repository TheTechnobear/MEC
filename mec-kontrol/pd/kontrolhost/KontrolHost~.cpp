#include "KontrolHost~.h"

#include "devices/Organelle.h"
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
static const int DEVICE_POLL_FREQUENCY = 50;




// puredata methods implementation - start

// helpers

static t_pd *get_object(const char *s) {
  t_pd *x = gensym(s)->s_thing;
  return x;
}


static void sendPdMessage(const char* obj, float f) {
  t_pd* sendobj = get_object(obj);
  if (!sendobj) { post("KontrolHost~::sendPdMessage to %s failed", obj); return; }

  t_atom a;
  SETFLOAT(&a, f);
  pd_forwardmess(sendobj, 1, &a);
}


/// main PD methods
t_int* KontrolHost_tilde_render(t_int *w)
{
  t_KontrolHost *x = (t_KontrolHost *)(w[1]);
  x->pollCount_++;
  if (x->osc_receiver_ && x->pollCount_ % OSC_POLL_FREQUENCY == 0) {
    x->osc_receiver_->poll();
  }

  if (x->device_ && x->pollCount_ % DEVICE_POLL_FREQUENCY == 0) {
    x->device_->poll();
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
  x->device_.reset();
  // Kontrol::ParameterModel::free();
}

void *KontrolHost_tilde_new(t_floatarg osc_in)
{
  t_KontrolHost *x = (t_KontrolHost *) pd_new(KontrolHost_tilde_class);


  x->osc_receiver_ = nullptr;

  x->pollCount_ = 0;
  x->param_model_ = Kontrol::ParameterModel::model();

  x->device_ = std::make_shared<Organelle>();
  x->device_->init();

  x->param_model_->addCallback("pd.send", std::make_shared<SendBroadcaster>());
  x->param_model_->addCallback("pd.client", std::make_shared<ClientHandler>(x));

  KontrolHost_tilde_listen(x, osc_in); // if zero will ignore
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


  class_addmethod(KontrolHost_tilde_class,
                  (t_method) KontrolHost_tilde_midiCC, gensym("midiCC"),
                  A_DEFFLOAT, A_DEFFLOAT, A_NULL);

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

void    KontrolHost_tilde_enc(t_KontrolHost* x, t_floatarg f) {
  if (x->device_) x->device_->changeEncoder(0, f);
}

void    KontrolHost_tilde_encbut(t_KontrolHost* x, t_floatarg f) {
  if (x->device_) x->device_->encoderButton(0, f);
}



void    KontrolHost_tilde_knob1Raw(t_KontrolHost* x, t_floatarg f) {
  if (x->device_) x->device_->changePot(0, f);}

void    KontrolHost_tilde_knob2Raw(t_KontrolHost* x, t_floatarg f) {
  if (x->device_) x->device_->changePot(1, f);
}

void    KontrolHost_tilde_knob3Raw(t_KontrolHost* x, t_floatarg f) {
  if (x->device_) x->device_->changePot(2, f);
}

void    KontrolHost_tilde_knob4Raw(t_KontrolHost* x, t_floatarg f) {
  if (x->device_) x->device_->changePot(3, f);
}


void    KontrolHost_tilde_midiCC(t_KontrolHost *x, t_floatarg cc, t_floatarg value) {
  if (x->device_) x->device_->midiCC((unsigned) cc, (unsigned) value);
}


void SendBroadcaster::changed(Kontrol::ParameterSource, const Kontrol::Parameter& param) {
  t_pd* sendobj = get_object(param.id().c_str());
  if (!sendobj) { post("send to %s failed", param.id().c_str()); return; }

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
  ;
}


// puredata methods implementation - end

