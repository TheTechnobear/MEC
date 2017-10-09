#include "OrganelleOLED.h"

#include "osc/OscOutboundPacketStream.h"
#include "KontrolHost~.h"

// #include <algorithm>
// #include <limits>

const unsigned int SCREEN_WIDTH = 21;

static const unsigned int OUTPUT_BUFFER_SIZE = 1024;
static char screenosc[OUTPUT_BUFFER_SIZE];


bool OrganelleOLED::connect() {
  try {
    socket_ =  std::shared_ptr<UdpTransmitSocket> ( new UdpTransmitSocket( IpEndpointName( "127.0.0.1", 4001 )));
  } catch (const std::runtime_error& e) {
    post("could not connect to mother host for screen updates");
    socket_.reset();
    return false;
  }
  return true;
}

void OrganelleOLED::displayPopup(const std::string& text, unsigned time) {
  popupTime_ = time;

  // CNMAT OSC used by mother exec, does not support bundles
#if 0
  std::string pad = "                   ";
  int fillc = (SCREEN_WIDTH - text.length() - 4 ) / 2;
  std::string disp = pad.substr(0, fillc) + "[ " + text + +" ]" + pad.substr(0, fillc);

  osc::OutboundPacketStream ops( screenosc, OUTPUT_BUFFER_SIZE );
  ops << osc::BeginMessage( "/oled/line/1" )
      << disp.c_str()
      << osc::EndMessage;
  socket_->Send( ops.Data(), ops.Size() );
#else
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
#endif


}

void OrganelleOLED::poll() {
  if (popupTime_ < 0) return;
  popupTime_--;
  if (popupTime_ == 0) {
    // release pop, redraw display


    auto pageId = x_->param_model_->getPageId(x_->currentPage_);
    if (pageId.empty()) return;
    for (int i = 1; i < 5; i++) {
      // parameters start from 0 on page, but line 1 is oled line
      // note: currently line 0 is unavailable, and 5 used for AUX
      auto id = x_->param_model_->getParamId(pageId, i - 1);
      if (id.empty()) return;
      auto param = x_->param_model_->getParam(id);
      if (param != nullptr) displayParamLine(i, *param);
    } // for

    // cancel timing
    popupTime_ = -1;
  }

}

std::string OrganelleOLED::asDisplayString(const Kontrol::Parameter& param, unsigned width) const {
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


void OrganelleOLED::displayParamLine(unsigned line, const Kontrol::Parameter& param) {
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


void OrganelleOLED::changed(Kontrol::ParameterSource src, const Kontrol::Parameter& param) {
  if (popupTime_ > 0) return;

  auto pageId = x_->param_model_->getPageId(x_->currentPage_);
  if (pageId.empty()) return;

  for (int i = 1; i < 5; i++) {
    // parameters start from 0 on page, but line 1 is oled line
    // note: currently line 0 is unavailable, and 5 used for AUX
    auto id = x_->param_model_->getParamId(pageId, i - 1);
    if (id.empty()) return;
    if ( id == param.id()) {
      displayParamLine(i, param);
      return;
    } // if id=param id
  } // for
}
