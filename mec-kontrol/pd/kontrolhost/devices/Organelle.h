#pragma once

#include "ParameterModel.h"

#include "KontrolDevice.h"
#include "ip/UdpSocket.h"
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
private:
  bool connect();

  std::string asDisplayString(const Kontrol::Parameter& p, unsigned width) const;
  std::shared_ptr<UdpTransmitSocket> socket_;
};
