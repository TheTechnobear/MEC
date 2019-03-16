#include <cassert>
#include <iostream>

#include <mec_prefs.h>
#include <mec_log.h>
#include <KontrolModel.h>

class LoggerCallback : public Kontrol::KontrolCallback {
public:
    void rack(Kontrol::ChangeSource, const Kontrol::Rack &rack) override {
        LOG_1("rack >> " << rack.id());
    }

    void module(Kontrol::ChangeSource, const Kontrol::Rack &rack, const Kontrol::Module &module) override {
        LOG_1("module >> " << rack.id() << "." << module.id());
    }

    void page(Kontrol::ChangeSource, const Kontrol::Rack &rack, const Kontrol::Module &module,
              const Kontrol::Page &page) override {
        LOG_1("page >> " << rack.id() << "." << module.id() << "." << page.id());
        // page.dump();
    }

    void param(Kontrol::ChangeSource, const Kontrol::Rack &rack, const Kontrol::Module &module,
               const Kontrol::Parameter &param) override {
        LOG_1("param >> " << rack.id() << "." << module.id() << "." << param.id());
        param.dump();
    }

    void changed(Kontrol::ChangeSource, const Kontrol::Rack &rack, const Kontrol::Module &module,
                 const Kontrol::Parameter &param) override {
        LOG_1("changed >> " << rack.id() << "." << module.id() << "." << param.id());
        param.dump();
    }

    void resource(Kontrol::ChangeSource, const Kontrol::Rack &rack,
                  const std::string &resType, const std::string &res) override {
        LOG_1("resource >> " << rack.id() << " " << resType << " " << res);
    }

    virtual void deleteRack(Kontrol::ChangeSource, const Kontrol::Rack &rack) override {
        LOG_1("deleteRack >> " << rack.id());
    }
};

int main(int argc, char **argv) {
    LOG_0("test kontrol started");
    std::string file;
    if (argc > 1) {
        file = argv[1];
    } else {
        file = "../resources/kontrol";
    }

    std::shared_ptr<Kontrol::KontrolModel> model = Kontrol::KontrolModel::model();

    auto cb = std::make_shared<LoggerCallback>();
    model->addCallback("logger", cb);

    std::string host = "localhost";
    unsigned port = 9001;

    Kontrol::EntityId rackId = Kontrol::Rack::createId(host, port);
    Kontrol::EntityId moduleId = "module1";
    model->createRack(Kontrol::CS_LOCAL, rackId, host, port);
    model->createModule(Kontrol::CS_LOCAL, rackId, moduleId, "Poly Synth", "polysynth");
    model->loadModuleDefinitions(rackId, moduleId, file + "-module.json");
    model->loadSettings(rackId, file + "-rack.json");

    for (auto rack : model->getRacks()) {
        rack->dumpParameters();
        rack->dumpSettings();

        // LOG_1("default values");
        // rack->dumpCurrentValues();

        LOG_1("preset 0: transpose 12, rmix 10");
        rack->loadPreset("0");
        // rack->dumpCurrentValues();

        LOG_1("preset 1: transpose 24");
        rack->loadPreset("1");
        // rack->dumpCurrentValues();

        LOG_1("cc 62, 80 : rmix 50");
        rack->changeMidiCC(1, 16); // do nothing, check doesnt crash ;)
        rack->changeMidiCC(62, 64);
        // rack->dumpCurrentValues();

        // rack->addMidiCCMapping(62, "module1", "o_level");
        // rack->updatePreset("2");
        // rack->saveSettings("./rack.json");
    }

    LOG_0("test completed");
    return 0;
}