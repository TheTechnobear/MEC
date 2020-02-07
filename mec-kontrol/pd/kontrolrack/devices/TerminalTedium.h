#pragma once

#include "KontrolDevice.h"

#include <KontrolModel.h>


#include <TTuiDevice.h>
#include <string>

class TerminalTedium : public KontrolDevice {
public:
    TerminalTedium();
    virtual ~TerminalTedium();

    //KontrolDevice
    virtual bool init() override;

    void displayPopup(const std::string &text,bool dblLine);
    void displayParamLine(unsigned line, const Kontrol::Parameter &p);
    void displayLine(unsigned line, const char *);
    void invertLine(unsigned line);
    void clearDisplay();
    void flipDisplay();

    void writePoll();
private:
    void send(const char *data, unsigned size);
    void stop() override ;

    std::string asDisplayString(const Kontrol::Parameter &p, unsigned width) const;

    TTuiLite::TTuiDevice device_;
};
