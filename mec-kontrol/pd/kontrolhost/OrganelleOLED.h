#pragma once

#include "ParameterModel.h"

#include "OrganelleOLED.h"
#include "ip/UdpSocket.h"

#include <string>

struct _KontrolHost;

class OrganelleOLED : public  Kontrol::ParameterCallback {
public:
  OrganelleOLED(_KontrolHost *x) : x_(x), popupTime_(-1) {
    connect();
  }

  bool connect();

  void displayPopup(const std::string& text, unsigned time);
  void poll();

  // ParameterCallback
  virtual void addClient(const std::string&, unsigned ) {;}
  virtual void page(Kontrol::ParameterSource , const Kontrol::Page& ) { ; } // not interested
  virtual void param(Kontrol::ParameterSource, const Kontrol::Parameter&) { ; } // not interested
  virtual void changed(Kontrol::ParameterSource src, const Kontrol::Parameter& p);
private:

  std::string asDisplayString(const Kontrol::Parameter& p, unsigned width) const;
  void displayParamLine(unsigned line, const Kontrol::Parameter& p);

  _KontrolHost * x_;
  std::shared_ptr<UdpTransmitSocket> socket_;
  int popupTime_;
};
