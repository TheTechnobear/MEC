#pragma once


class NuiMenuMode : public NuiBaseMode {
public:
    explicit NuiMenuMode(Nui &p) : NuiBaseMode(p), cur_(0), top_(0) { ; }

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


class NuiFixedMenuMode : public NuiMenuMode {
public:
    explicit NuiFixedMenuMode(Nui &p) : NuiMenuMode(p) { ; }

    unsigned getSize() override { return items_.size(); };

    std::string getItemText(unsigned i) override { return items_[i]; }

protected:
    std::vector<std::string> items_;
};

class NuiMainMenu : public NuiMenuMode {
public:
    explicit NuiMainMenu(Nui &p) : NuiMenuMode(p) { ; }

    bool init() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
    void activeModule(Kontrol::ChangeSource, const Kontrol::Rack &, const Kontrol::Module &) override;
};

class NuiPresetMenu : public NuiMenuMode {
public:
    explicit NuiPresetMenu(Nui &p) : NuiMenuMode(p) { ; }

    bool init() override;
    void activate() override;
    unsigned getSize() override;
    std::string getItemText(unsigned idx) override;
    void clicked(unsigned idx) override;
private:
    std::vector<std::string> presets_;
};


class NuiModuleMenu : public NuiFixedMenuMode {
public:
    explicit NuiModuleMenu(Nui &p) : NuiFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
private:
    void populateMenu(const std::string& catSel);
    std::string cat_;
};

class NuiModuleSelectMenu : public NuiFixedMenuMode {
public:
    explicit NuiModuleSelectMenu(Nui &p) : NuiFixedMenuMode(p) { ; }

    void activate() override;
    void clicked(unsigned idx) override;
};
