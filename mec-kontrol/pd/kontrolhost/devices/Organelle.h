#pragma once

#include "KontrolDevice.h"

#include <ParameterModel.h>
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
  virtual void changed(Kontrol::ParameterSource src, const Kontrol::Parameter& p);

  std::string currentModule() { return currentModule_;}
  void currentModule(const std::string& p) { currentModule_ = p;}


private:
  bool connect();
  std::string currentModule_;
  std::string lastParamId_; // for midi learn
  bool midiLearnActive_;

  std::string asDisplayString(const Kontrol::Parameter& p, unsigned width) const;
  std::shared_ptr<UdpTransmitSocket> socket_;
};
