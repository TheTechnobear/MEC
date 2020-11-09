#pragma once

#include "e1_basemode.h"

namespace mec {


class ElectraOneMenuMode : public ElectraOneBaseMode {
public:
    explicit ElectraOneMenuMode(ElectraOne &p) : ElectraOneBaseMode(p), cur_(0), top_(0) { ; }

    virtual unsigned getSize() = 0;
    virtual std::string getItemText(unsigned idx) = 0;
    virtual void clicked(unsigned idx) = 0;

    bool init() override { return true; }

    void poll() override;
    void activate() override;
    virtual void navPrev();
    virtual void navNext();
    virtual void navActivate();

    void onButton(unsigned id, unsigned value) override;
    void onEncoder(unsigned id, int value) override;

    void savePreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;
    void loadPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) override;
    void midiLearn(Kontrol::ChangeSource src, bool b) override;
    void modulationLearn(Kontrol::ChangeSource src, bool b) override;
protected:
    void display();
    void displayItem(unsigned idx);
    unsigned cur_;
    unsigned top_;
};


class ElectraOneFixedMenuMode : public ElectraOneMenuMode {
public:
    explicit ElectraOneFixedMenuMode(ElectraOne &p) : ElectraOneMenuMode(p) { ; }

    unsigned getSize() override { return items_.size(); };

    std::string getItemText(unsigned i) override { return items_[i]; }

protected:
    std::vector<std::string> items_;
};

class ElectraOneMainMenu : public ElectraOneMenuMode {
public:
    explicit ElectraOneMainMenu(ElectraOne &p) : ElectraOneMenuMode(p) { ; }

    bool init() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
};

class ElectraOnePresetMenu : public ElectraOneMenuMode {
public:
    explicit ElectraOnePresetMenu(ElectraOne &p) : ElectraOneMenuMode(p) { ; }

    bool init() override;
    void activate() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
private:
    std::vector<std::string> presets_;
};


class ElectraOneModuleMenu : public ElectraOneFixedMenuMode {
public:
    explicit ElectraOneModuleMenu(ElectraOne &p) : ElectraOneFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
private:
    void populateMenu(const std::string &catSel);
    std::string cat_;
};

class ElectraOneModuleSelectMenu : public ElectraOneFixedMenuMode {
public:
    explicit ElectraOneModuleSelectMenu(ElectraOne &p) : ElectraOneFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
};


} // mec
