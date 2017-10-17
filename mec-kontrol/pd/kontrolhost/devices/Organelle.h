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
  virtual void poll();
  virtual void changePot(unsigned pot, float value);
  virtual void changeEncoder(unsigned encoder, float value);
  virtual void encoderButton(unsigned encoder, bool value);

  void displayPopup(const std::string& text);
  void displayParamLine(unsigned line, const Kontrol::Parameter& p);
private:
  bool connect();

  std::string asDisplayString(const Kontrol::Parameter& p, unsigned width) const;
  std::shared_ptr<UdpTransmitSocket> socket_;
};
