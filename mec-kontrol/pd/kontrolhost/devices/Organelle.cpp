#include "Organelle.h"

#include "osc/OscOutboundPacketStream.h"

#include <iostream>
#include "m_pd.h"
// #include <algorithm>
// #include <limits>

const unsigned int SCREEN_WIDTH = 21;

static const int PAGE_SWITCH_TIMEOUT = 5;
static const int PAGE_EXIT_TIMEOUT = 5;
static const int MENU_TIMEOUT = 35;


static const unsigned int OUTPUT_BUFFER_SIZE = 1024;
static char screenosc[OUTPUT_BUFFER_SIZE];

static const unsigned MAX_POT_VALUE = 1023;

enum OrganelleModes {
  OM_PARAMETER,
  OM_MAINMENU,
  OM_PRESETMENU
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
  virtual void activate();
  virtual void changePot(unsigned pot, float value);
  virtual void changeEncoder(unsigned encoder, float value);
  virtual void encoderButton(unsigned encoder, bool value);
  virtual void changed(Kontrol::ParameterSource src, const Kontrol::Parameter& p);
private:
  void display();
  std::string get_param_id(unsigned paramnum);
  std::shared_ptr<Pots> pots_;
  unsigned currentPage_;
};

class OMenuMode : public OBaseMode {
public:
  OMenuMode(Organelle& p) : OBaseMode(p), cur_(0), top_(0) {;}
  virtual unsigned getSize() = 0;
  virtual std::string getItemText(unsigned idx) = 0;
  virtual void clicked(unsigned idx) = 0;

  virtual bool init() { return true;}
  virtual void poll();
  virtual void activate();
  virtual void changeEncoder(unsigned encoder, float value);
  virtual void encoderButton(unsigned encoder, bool value);

protected:
  void display();
  void displayItem(unsigned idx);
  unsigned cur_;
  unsigned top_;
};


class OFixedMenuMode : public OMenuMode {
public:
  OFixedMenuMode(Organelle& p) : OMenuMode(p) {;}
  virtual unsigned getSize() { return items_.size(); };
  virtual std::string getItemText(unsigned i)  { return items_[i];}

protected:
  std::vector<std::string> items_;
};

class OMainMenu : public OMenuMode {
public:
  OMainMenu(Organelle& p) : OMenuMode(p) {;}
  virtual bool init();
  virtual unsigned getSize();
  virtual std::string getItemText(unsigned idx);
  virtual void clicked(unsigned idx);
};

class OPresetMenu : public OMenuMode {
public:
  OPresetMenu(Organelle& p) : OMenuMode(p) {;}
  virtual bool init();
  virtual void activate();
  virtual unsigned getSize();
  virtual std::string getItemText(unsigned idx);
  virtual void clicked(unsigned idx);
private:
  std::vector<std::string> presets_;
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

void OParamMode::display() {
  auto pageId = model()->getPageId(currentPage_);
  if (pageId.empty()) return;
  parent_.clearDisplay();
  for (int i = 1; i < 5; i++) {
    // parameters start from 0 on page, but line 1 is oled line
    // note: currently line 0 is unavailable, and 5 used for AUX
    auto id = model()->getParamId(pageId, i - 1);
    if (id.empty()) return;
    auto param = model()->getParam(id);
    if (param != nullptr) parent_.displayParamLine(i, *param);
  } // for
}


void OParamMode::activate()  {
  display();
}


void OParamMode::poll() {
  OBaseMode::poll();
  // release pop, redraw display
  if (popupTime_ == 0) {
    display();

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
        if (calc > param->current()) {
          pots_->locked_[pot] = Pots::K_UNLOCKED;
          //std::cout << "unlock condition met gt " << pot << std::endl;
        }
      }
      else if (pots_->locked_[pot] == Pots::K_LT) {
        if (calc < param->current()) {
          pots_->locked_[pot] = Pots::K_UNLOCKED;
          //std::cout << "unlock condition met lt " << pot << std::endl;
        }
      }
      else if (pots_->locked_[pot] == Pots::K_LOCKED) {
        // initial locked, determine unlock condition
        if (calc > param->current()) {
          // pot starts greater than param, so wait for it to go less than
          pots_->locked_[pot] = Pots::K_LT;
          //std::cout << "set unlock condition lt " << pot << std::endl;
        } else {
          // pot starts less than param, so wait for it to go greater than
          pots_->locked_[pot] = Pots::K_GT;
          //std::cout << "set unlock condition gt " << pot << std::endl;
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
  if (value < 1.0) {
    parent_.changeMode(OM_MAINMENU);
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
        //std::cout << "locking " << param.id() << " src " << src << std::endl;
        pots_->locked_[i - 1] = Pots::K_LOCKED;
      }
      return;
    } // if id=param id
  } // for
}

void OMenuMode::activate() {
  display();
  popupTime_ = MENU_TIMEOUT;
}

void OMenuMode::poll() {
  OBaseMode::poll();
  if (popupTime_ == 0) {
    parent_.changeMode(OM_PARAMETER);
    popupTime_ = -1;
  }
}

void OMenuMode::display() {
  parent_.clearDisplay();
  for (unsigned i = top_; i < top_ + 4; i++) {
    displayItem(i);
  }
}

void OMenuMode::displayItem(unsigned i) {
  if (i < getSize()) {
    std::string item = getItemText(i);
    unsigned line = i - top_ + 1;
    parent_.displayLine(line, item.c_str());
    if ( i == cur_) {
      parent_.invertLine(line);
    }
  }
}


void OMenuMode::changeEncoder(unsigned encoder, float value) {
  unsigned cur = cur_;
  if (value > 0) {
    // clockwise
    cur++;
    cur = std::min(cur, (unsigned) getSize() - 1);
  } else {
    // anti clockwise
    if (cur > 0) cur--;
  }
  if (cur != cur_) {
    int line = 0;
    if (cur < top_ ) {
      top_ = cur;
      cur_ = cur;
      display();
    } else if (cur >= top_ + 4) {
      top_ = cur - 3;
      cur_ = cur;
      display();
    }
    else {
      line = cur_ - top_ + 1;
      if (line >= 0 && line <= 4) parent_.invertLine(line);
      cur_ = cur;
      line = cur_ - top_ + 1;
      if (line >= 0 && line <= 4) parent_.invertLine(line);
    }
  }
  popupTime_ = MENU_TIMEOUT;
}


void OMenuMode::encoderButton(unsigned encoder, bool value) {
  if (value < 1.0)  {
    clicked(cur_);
  }
}


/// main menu
enum MainMenuItms {
  MMI_HOME,
  MMI_MODULE,
  MMI_PRESET,
  MMI_LEARN,
  MMI_SAVE,
  MMI_SIZE
};

bool OMainMenu::init() {
  return true;
}


unsigned OMainMenu::getSize() {
  return (unsigned) MMI_SIZE;
}

std::string OMainMenu::getItemText(unsigned idx) {
  switch (idx) {
  case MMI_HOME:   return "Home";
  case MMI_MODULE: return parent_.currentModule();
  case MMI_PRESET: return model()->currentPreset();
  case MMI_SAVE:   return "Save Settings";
  case MMI_LEARN:  {
    if (parent_.midiLearn()) {
      return "Midi Learn        [X]";
    }
    return "Midi Learn        [ ]";
  }
  default: break;
  }
  return "";
}


void OMainMenu::clicked(unsigned idx) {
  switch (idx) {
  case MMI_HOME:   {
    parent_.changeMode(OM_PARAMETER);
    parent_.sendPdMessage("goHome", 1.0);
    break;
  }
  case MMI_MODULE: {
    parent_.changeMode(OM_PARAMETER);
    break;
  }
  case MMI_PRESET: {
    parent_.changeMode(OM_PRESETMENU);
    break;
  }
  case MMI_LEARN:  {
    parent_.midiLearn(!parent_.midiLearn());
    displayItem(MMI_LEARN);
    // parent_.changeMode(OM_PARAMETER);
    break;
  }
  case MMI_SAVE:  {
    model()->savePatchSettings();
    parent_.changeMode(OM_PARAMETER);
    break;
  }
  default: break;
  }
}

// preset menu
enum PresetMenuItms {
  PMI_SAVE,
  PMI_NEW,
  PMI_SEP,
  PMI_LAST
};


bool OPresetMenu::init() {
  presets_ = model()->getPresetList();
  return true;
}

void OPresetMenu::activate() {
  presets_ = model()->getPresetList();
  OMenuMode::activate();
}


unsigned OPresetMenu::getSize() {
  return (unsigned) PMI_LAST + presets_.size();
}

std::string OPresetMenu::getItemText(unsigned idx) {
  switch (idx) {
  case PMI_SAVE:  return "Save Preset";
  case PMI_NEW:   return "New Preset";
  case PMI_SEP:   return "--------------------";
  default:
    return presets_[idx - PMI_LAST];
  }
  return "";
}


void OPresetMenu::clicked(unsigned idx) {
  switch (idx) {
  case PMI_SAVE:   {
    model()->savePreset(model()->currentPreset());
    parent_.changeMode(OM_MAINMENU);
    break;
  }
  case PMI_NEW:   {
    model()->savePreset("New " + std::to_string(presets_.size()));
    parent_.changeMode(OM_MAINMENU);
    break;
  }
  case PMI_SEP: {
    break;
  }
  default: {
    model()->applyPreset(presets_[idx - PMI_LAST]);
    break;
  }
  }
}




// Organelle implmentation

Organelle::Organelle() {
  ;
}



bool Organelle::init() {
  // add modes before KD init
  addMode(OM_PARAMETER, std::make_shared<OParamMode>(*this));
  addMode(OM_MAINMENU, std::make_shared<OMainMenu>(*this));
  addMode(OM_PRESETMENU, std::make_shared<OPresetMenu>(*this));

  if (KontrolDevice::init()) {
    currentModule("Module 1");
    midiLearn(false);
    lastParamId_ = "";

    // setup mother.pd for reasonable behaviour, basically takeover
    sendPdMessage("midiOutGate", 0.0f);
    // sendPdMessage("midiInGate",0.0f);
    sendPdMessage("enableSubMenu", 1.0f);
    connect();
    changeMode(OM_PARAMETER);
    return true;
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

void Organelle::displayPopup(const std::string & text) {
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



std::string Organelle::asDisplayString(const Kontrol::Parameter & param, unsigned width) const {
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

void Organelle::clearDisplay() {
  osc::OutboundPacketStream ops( screenosc, OUTPUT_BUFFER_SIZE );
  //ops << osc::BeginMessage( "/oled/gClear" )
  //    << 1
  //    << osc::EndMessage;
  ops << osc::BeginMessage( "/oled/gFillArea" )
      << 127 << 45 << 0 << 8 << 0
      << osc::EndMessage;
  socket_->Send( ops.Data(), ops.Size() );
}

void Organelle::displayParamLine(unsigned line, const Kontrol::Parameter & param) {
  std::string disp = asDisplayString(param, SCREEN_WIDTH);
  displayLine(line, disp.c_str());
}

void Organelle::displayLine(unsigned line, const char* disp) {
  if (socket_ == nullptr) return;

  int x =  ((line - 1) * 11) + ((line > 0) * 9 );
  {
    osc::OutboundPacketStream ops( screenosc, OUTPUT_BUFFER_SIZE );
    ops << osc::BeginMessage( "/oled/gFillArea" )
        << 127 << 10 << 0 << x << 0
        << osc::EndMessage;
    socket_->Send( ops.Data(), ops.Size() );
  }
  {
    osc::OutboundPacketStream ops( screenosc, OUTPUT_BUFFER_SIZE );
    ops << osc::BeginMessage( "/oled/gPrintln" )
        << 2 << x << 8 << 1
        << disp
        << osc::EndMessage;
    socket_->Send( ops.Data(), ops.Size() );
  }



}

void Organelle::invertLine(unsigned line) {
  osc::OutboundPacketStream ops( screenosc, OUTPUT_BUFFER_SIZE );

  int x =  ((line - 1) * 11) + ((line > 0) * 9 );
  ops << osc::BeginMessage( "/oled/gInvertArea" )
      << 127 << 10 << 0 << x - 1
      << osc::EndMessage;

  socket_->Send( ops.Data(), ops.Size() );

}

void Organelle::changed(Kontrol::ParameterSource src, const Kontrol::Parameter & p) {
  if (midiLearnActive_) {
    lastParamId_ = p.id();
  }
  KontrolDevice::changed(src, p);
}

void Organelle::midiLearn(bool b) {
  lastParamId_ = "";
  midiLearnActive_ = b;
}


void Organelle::midiCC(unsigned num, unsigned value) {
  //std::cout << "midiCC " << num << " " << value << std::endl;
  if (midiLearnActive_) {
    if (!lastParamId_.empty()) {
      if (value > 0) {
        model()->addMidiCCMapping(num, lastParamId_);
        lastParamId_ = "";
      }
      else {
        model()->addMidiCCMapping(num, "");
      }
    }
  }
  // update param model
  KontrolDevice::midiCC(num, value);
}




