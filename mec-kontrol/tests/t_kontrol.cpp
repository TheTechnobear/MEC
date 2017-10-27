#include <cassert>
#include <iostream>

#include <mec_prefs.h>
#include <mec_log.h>
#include <KontrolModel.h>

class LoggerCallback : public Kontrol::KontrolCallback {
public:
    virtual void rack(Kontrol::ParameterSource, const Kontrol::Rack& rack)  {
        LOG_1("rack >> " << rack.id());
    }

    virtual void module(Kontrol::ParameterSource, const Kontrol::Rack& rack, const Kontrol::Module& module) {
        LOG_1("module >> " << rack.id() << "." << module.id());
    }

    virtual void page(Kontrol::ParameterSource, const Kontrol::Rack& rack, const Kontrol::Module& module, const Kontrol::Page& page) {
        LOG_1("page >> " << rack.id() << "." << module.id() << "." << page.id());
        // page.dump();
    }

    virtual void param(Kontrol::ParameterSource, const Kontrol::Rack& rack, const Kontrol::Module& module, const Kontrol::Parameter& param) {
        LOG_1("param >> " << rack.id() << "." << module.id() << "." << param.id());
        param.dump();
    }

    virtual void changed(Kontrol::ParameterSource, const Kontrol::Rack& rack, const Kontrol::Module& module, const Kontrol::Parameter& param) {
        LOG_1("changed >> " << rack.id() << "." << module.id() << "." << param.id());
        param.dump();
    }
};

int main (int argc, char** argv) {
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
    unsigned port = 9000;

    Kontrol::EntityId rackId = Kontrol::Rack::createId(host, port);
    Kontrol::EntityId moduleId = "module1";
    model->createRack(Kontrol::PS_LOCAL, rackId, host, port);
    model->createModule(Kontrol::PS_LOCAL, rackId, moduleId, "Poly Synth", "polysynth");
    model->loadModuleDefinitions(rackId, moduleId, file + "-module.json");
    model->loadSettings(rackId, file + "-rack.json");

    for (auto rack : model->getRacks()) {
        rack->dumpParameters();
        rack->dumpSettings();

        // LOG_1("default values");
        // rack->dumpCurrentValues();

        LOG_1("preset 0: transpose 12, rmix 10");
        rack->applyPreset("0");
        // rack->dumpCurrentValues();

        LOG_1("preset 1: transpose 24");
        rack->applyPreset("1");
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