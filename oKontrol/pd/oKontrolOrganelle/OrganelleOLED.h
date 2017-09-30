#pragma once

#include "ParameterModel.h"

#include "OrganelleOLED.h"
#include "ip/UdpSocket.h"

#include <string>

struct _oKontrolOrganelle;

class OrganelleOLED : public  oKontrol::ParameterCallback {
public:
  OrganelleOLED(_oKontrolOrganelle *x) : x_(x) {
    connect();
  }
  bool connect();

  // ParameterCallback
  virtual void addClient(const std::string&, unsigned ) {;}
  virtual void page(oKontrol::ParameterSource , const oKontrol::Page& ) { ; } // not interested
  virtual void param(oKontrol::ParameterSource, const oKontrol::Parameter&) { ; } // not interested
  virtual void changed(oKontrol::ParameterSource src, const oKontrol::Parameter& p);
private:
  std::string asDisplayString(const oKontrol::Parameter& p, unsigned width) const;

  _oKontrolOrganelle * x_;
  std::shared_ptr<UdpTransmitSocket> socket_;
};
