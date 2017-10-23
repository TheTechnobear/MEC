#include <cassert>
#include <iostream>

#include <mec_prefs.h>
#include <mec_log.h>
#include <KontrolModel.h>

int main (int argc, char** argv) {
    LOG_0("test kontrol started");

    std::shared_ptr<Kontrol::KontrolModel> model = Kontrol::KontrolModel::model();
    std::string host = "localhost";
    unsigned port = 9000;

    Kontrol::EntityId rackId=Kontrol::Rack::createId(host,port);
    Kontrol::EntityId moduleId="module1";
    model->createRack(Kontrol::PS_LOCAL,rackId,host,port);
    model->createModule(Kontrol::PS_LOCAL,rackId,moduleId,"Poly Synth", "polysynth");
    model->loadModuleDefinitions(rackId,moduleId,"../resources/kontrol-module.json");
    model->loadSettings(rackId,"../resources/kontrol-rack.json");

    for(auto rack: model->getRacks()) {
        rack->dumpParameters();
        rack->dumpSettings();

        LOG_1("default values");
        rack->dumpCurrentValues();

        rack->applyPreset("0");
        LOG_1("preset 0: transpose 12, rmix 10");
        rack->dumpCurrentValues();

        rack->applyPreset("1");
        LOG_1("preset 1: transpose 24");
        rack->dumpCurrentValues();

        LOG_1("cc 62, 80 : rmix 50");
        rack->changeMidiCC(1, 16); // do nothing, check doesnt crash ;)
        rack->changeMidiCC(62, 64);
        rack->dumpCurrentValues();

        rack->addMidiCCMapping(62, "module1", "o_level");
        rack->updatePreset("2");
        rack->saveSettings("./rack.json");
    }

    LOG_0("test completed");
    return 0;
}