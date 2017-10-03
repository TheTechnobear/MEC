#pragma once

#include "ParameterModel.h"

#include "OrganelleOLED.h"
#include "ip/UdpSocket.h"

#include <string>

struct _oKontrolOrganelle;

class OrganelleOLED : public  oKontrol::ParameterCallback {
public:
  OrganelleOLED(_oKontrolOrganelle *x) : x_(x), popupTime_(-1) {
    connect();
  }

  bool connect();

  void displayPopup(const std::string& text, unsigned time);
  void poll();

  // ParameterCallback
  virtual void addClient(const std::string&, unsigned ) {;}
  virtual void page(oKontrol::ParameterSource , const oKontrol::Page& ) { ; } // not interested
  virtual void param(oKontrol::ParameterSource, const oKontrol::Parameter&) { ; } // not interested
  virtual void changed(oKontrol::ParameterSource src, const oKontrol::Parameter& p);
private:

  std::string asDisplayString(const oKontrol::Parameter& p, unsigned width) const;
  void displayParamLine(unsigned line, const oKontrol::Parameter& p);

  _oKontrolOrganelle * x_;
  std::shared_ptr<UdpTransmitSocket> socket_;
  int popupTime_;
};
