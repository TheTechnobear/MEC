#include "Organelle.h"

#include "osc/OscOutboundPacketStream.h"

#include "m_pd.h"
// #include <algorithm>
// #include <limits>

const unsigned int SCREEN_WIDTH = 21;

static const int PAGE_SWITCH_TIMEOUT = 5;
static const int PAGE_EXIT_TIMEOUT = 5;


static const unsigned int OUTPUT_BUFFER_SIZE = 1024;
static char screenosc[OUTPUT_BUFFER_SIZE];

static const unsigned MAX_POT_VALUE = 1023;

enum OrganelleModes {
  OM_PARAMETER
};


class OBaseMode : public DeviceMode {
public:
  OBaseMode(Organelle& p) : parent_(p), popupTime_(-1) {;}
  virtual bool init() { return true;}
  virtual void poll();

  virtual void changePot(unsigned, float ) {;}
  virtual void changeEncoder(unsigned, float ) {;}
  virtual void encoderButton(unsigned, bool ) {;}
  virtual void addClient(const std::string&, unsigned ) {;}
  virtual void page(Kontrol::ParameterSource , const Kontrol::Page& )  {;}
  virtual void param(Kontrol::ParameterSource, const Kontrol::Parameter&) {;}
  virtual void changed(Kontrol::ParameterSource src, const Kontrol::Parameter& p)  {;}

  void displayPopup(const std::string& text, unsigned time);
protected:
  Organelle& parent_;
  std::shared_ptr<Kontrol::ParameterModel> model() { return parent_.model();}
  int popupTime_;
};

struct Pots {
  enum {
    K_UNLOCKED,
    K_GT,
    K_LT,
    K_LOCKED
  } locked_[4];
};



class OParamMode : public OBaseMode {
public:
  OParamMode(Organelle& p) : OBaseMode(p), currentPage_(0) {;}
  virtual bool init();
  virtual void poll();
  virtual void changePot(unsigned pot, float value);
  virtual void changeEncoder(unsigned encoder, float value);
  virtual void encoderButton(unsigned encoder, bool value);
  virtual void changed(Kontrol::ParameterSource src, const Kontrol::Parameter& p);
private:
  std::string get_param_id(unsigned paramnum);
  std::shared_ptr<Pots> pots_;
  unsigned currentPage_;
};

void OBaseMode::displayPopup(const std::string& text, unsigned time) {
  popupTime_ = time;
  parent_.displayPopup(text);
}

void OBaseMode::poll() {
  if (popupTime_ < 0) return;
  popupTime_--;
}



bool OParamMode::init() {
  OBaseMode::init();
  pots_ = std::make_shared<Pots>();
  for (int i = 0; i < 4; i++) {
    pots_->locked_[i] = Pots::K_UNLOCKED;
  }
  return true;
}

void OParamMode::poll() {
  OBaseMode::poll();
  // release pop, redraw display
  if (popupTime_ == 0) {
    auto pageId = model()->getPageId(currentPage_);
    if (pageId.empty()) return;
    for (int i = 1; i < 5; i++) {
      // parameters start from 0 on page, but line 1 is oled line
      // note: currently line 0 is unavailable, and 5 used for AUX
      auto id = model()->getParamId(pageId, i - 1);
      if (id.empty()) return;
      auto param = model()->getParam(id);
      if (param != nullptr) parent_.displayParamLine(i, *param);
    } // for

    // cancel timing
    popupTime_ = -1;
  }
}

std::string OParamMode::get_param_id(unsigned paramnum) {
  auto pageId = model()->getPageId(currentPage_);
  if (pageId.empty()) return "";
  auto page = model()->getPage(pageId);
  if (page == nullptr) return "";
  auto id = model()->getParamId(page->id(), paramnum);
  return id;
}


void OParamMode::changePot(unsigned pot, float value) {
  OBaseMode::changePot(pot, value);
  auto id = get_param_id(pot);
  auto param = model()->getParam(id);
  if (param == nullptr) return;
  if (!id.empty())  {
    Kontrol::ParamValue calc = param->calcFloat(value / MAX_POT_VALUE);
    if (pots_->locked_[pot] != Pots::K_UNLOCKED) {
      //if pot is locked, determined if we can unlock it
      if (calc == param->current()) {
        pots_->locked_[pot] = Pots::K_UNLOCKED;
      }
      else if (pots_->locked_[pot] == Pots::K_GT) {
        if (calc > param->current()) pots_->locked_[pot] = Pots::K_UNLOCKED;
      }
      else if (pots_->locked_[pot] == Pots::K_LT) {
        if (calc < param->current()) pots_->locked_[pot] = Pots::K_UNLOCKED;
      }
      else if (pots_->locked_[pot] == Pots::K_LOCKED) {
        // initial locked, determine unlock condition
        if (calc > param->current()) {
          // pot starts greater than param, so wait for it to go less than
          pots_->locked_[pot] = Pots::K_LT;
        } else {
          // pot starts less than param, so wait for it to go greater than
          pots_->locked_[pot] = Pots::K_GT;
        }
      }
    }

    if (pots_->locked_[pot] == Pots::K_UNLOCKED) {
      model()->changeParam(Kontrol::PS_LOCAL, id, calc);
    }
  }
}


void OParamMode::changeEncoder(unsigned enc, float value) {
  OBaseMode::changeEncoder(enc, value);
  unsigned pagenum = currentPage_;
  if (value > 0) {
    // clockwise
    pagenum++;
    pagenum = std::min(pagenum, model()->getPageCount() - 1);
  } else {
    // anti clockwise
    if (pagenum > 0) pagenum--;
  }

  if (pagenum != currentPage_) {
    auto pageId = model()->getPageId(pagenum);
    if (pageId.empty()) return;
    auto page = model()->getPage(pageId);
    if (page == nullptr) return;
    displayPopup(page->displayName(), PAGE_SWITCH_TIMEOUT);

    currentPage_ = pagenum;
    for (int i = 0; i < 4; i++) {
      pots_->locked_[i] = Pots::K_LOCKED;
    }
  }
}

void OParamMode::encoderButton(unsigned enc, bool value) {
  OBaseMode::encoderButton(enc, value);
  post("encoder button %f", value);
  if (value > 0) {
    displayPopup("exit", PAGE_EXIT_TIMEOUT);
    parent_.sendPdMessage("goHome", 1.0);
  }
}


void OParamMode::changed(Kontrol::ParameterSource src, const Kontrol::Parameter& param) {
  OBaseMode::changed(src, param);
  if (popupTime_ > 0) return;

  auto pageId = model()->getPageId(currentPage_);
  if (pageId.empty()) return;

  for (int i = 1; i < 5; i++) {
    // parameters start from 0 on page, but line 1 is oled line
    // note: currently line 0 is unavailable, and 5 used for AUX
    auto id = model()->getParamId(pageId, i - 1);
    if (id.empty()) return;
    if ( id == param.id()) {
      parent_.displayParamLine(i, param);
      if (src != Kontrol::PS_LOCAL) {
        pots_->locked_[i - 1] = Pots::K_LOCKED;
      }
      return;
    } // if id=param id
  } // for
}



// Organelle implmentation

Organelle::Organelle() {
  ;
}



bool Organelle::init() {
  // add modes before KD init
  addMode(OM_PARAMETER, std::make_shared<OParamMode>(*this));
  changeMode(OM_PARAMETER);

  if (KontrolDevice::init()) {
    // setup mother.pd for reasonable behaviour, basically takeover
    sendPdMessage("midiOutGate", 0.0f);
    // sendPdMessage("midiInGate",0.0f);
    sendPdMessage("enableSubMenu", 1.0f);
    return connect();
  }
  return false;
}


bool Organelle::connect() {
  try {
    socket_ =  std::shared_ptr<UdpTransmitSocket> ( new UdpTransmitSocket( IpEndpointName( "127.0.0.1", 4001 )));
  } catch (const std::runtime_error& e) {
    post("could not connect to mother host for screen updates");
    socket_.reset();
    return false;
  }
  return true;
}

void Organelle::displayPopup(const std::string& text) {
  {
    osc::OutboundPacketStream ops( screenosc, OUTPUT_BUFFER_SIZE );
    ops << osc::BeginMessage( "/oled/gFillArea" )
        << 100 << 34 << 14 << 14 << 0
        << osc::EndMessage;
    socket_->Send( ops.Data(), ops.Size() );
  }

  {
    osc::OutboundPacketStream ops( screenosc, OUTPUT_BUFFER_SIZE );
    ops << osc::BeginMessage( "/oled/gBox" )
        << 100 << 34 << 14 << 14 << 1
        << osc::EndMessage;
    socket_->Send( ops.Data(), ops.Size() );
  }

  {
    osc::OutboundPacketStream ops( screenosc, OUTPUT_BUFFER_SIZE );
    ops << osc::BeginMessage( "/oled/gPrintln" )
        << 20 << 24 << 16 << 1
        << text.c_str()
        << osc::EndMessage;
    socket_->Send( ops.Data(), ops.Size() );
  }
}



std::string Organelle::asDisplayString(const Kontrol::Parameter& param, unsigned width) const {
  std::string pad = "";
  std::string ret;
  std::string value = param.displayValue();
  std::string unit = std::string(param.displayUnit() + "  ").substr(0, 2);
  const std::string& dName = param.displayName();
  int fillc = width - (dName.length() + value.length() + 1 + unit.length());
  for (; fillc > 0; fillc--) pad += " ";
  ret = dName + pad + value + " " + unit;
  if (ret.length() > width) ret = ret.substr(width - ret.length(), width);
  return ret;
}


void Organelle::displayParamLine(unsigned line, const Kontrol::Parameter& param) {
  static const char* oledLine0 = "/oled/line/0";
  static const char* oledLine1 = "/oled/line/1";
  static const char* oledLine2 = "/oled/line/2";
  static const char* oledLine3 = "/oled/line/3";
  static const char* oledLine4 = "/oled/line/4";
  static const char* oledLine5 = "/oled/line/5";

  const char* msg = oledLine1;
  switch (line) {
  case 0: msg = oledLine0; break;
  case 1: msg = oledLine1; break;
  case 2: msg = oledLine2; break;
  case 3: msg = oledLine3; break;
  case 4: msg = oledLine4; break;
  case 5: msg = oledLine5; break;
  default:
    msg = oledLine1;
  }

  std::string disp = asDisplayString(param, SCREEN_WIDTH);
  osc::OutboundPacketStream ops( screenosc, OUTPUT_BUFFER_SIZE );

  // CNMAT OSC used by mother exec, does not support bundles
  ops << osc::BeginMessage( msg )
      << disp.c_str()
      << osc::EndMessage;

  socket_->Send( ops.Data(), ops.Size() );

}






