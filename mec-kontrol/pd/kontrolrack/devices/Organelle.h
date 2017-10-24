#pragma once

#include "KontrolDevice.h"

#include <KontrolModel.h>
#include <ip/UdpSocket.h>
#include <string>



class Organelle : public  KontrolDevice {
public:
  Organelle();

  //KontrolDevice
  virtual bool init();

  void displayPopup(const std::string& text);
  void displayParamLine(unsigned line, const Kontrol::Parameter& p);
  void displayLine(unsigned line, const char*);
  void invertLine(unsigned line);
  void clearDisplay();

  void midiCC(unsigned num, unsigned value);
  void midiLearn(bool b);
  bool midiLearn() { return midiLearnActive_;}

  virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack&, const Kontrol::Module&, const Kontrol::Parameter&);

  Kontrol::EntityId currentRack() { return currentRackId_;}
  void              currentRack(const Kontrol::EntityId & p) { currentRackId_ = p;}
  Kontrol::EntityId currentModule() { return currentModuleId_;}
  void              currentModule(const Kontrol::EntityId & p) { currentModuleId_ = p;}
  std::string       currentPreset() { return currentPreset_;}
  void              currentPreset(const std::string& p) { currentPreset_ = p;}


private:
  bool connect();
  Kontrol::EntityId currentRackId_;
  Kontrol::EntityId currentModuleId_;
  Kontrol::EntityId lastParamId_; // for midi learn
  std::string       currentPreset_;
  bool midiLearnActive_;

  std::string asDisplayString(const Kontrol::Parameter& p, unsigned width) const;
  std::shared_ptr<UdpTransmitSocket> socket_;
};
