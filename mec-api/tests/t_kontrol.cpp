#include <mec_api.h>

#include <cassert>
#include <iostream>

#include <mec_voice.h>
#include <mec_prefs.h>
#include <mec_log.h>
#include "ParameterModel.h"

int main (int argc, char** argv) {
    LOG_0("test kontrol started");

    mec::Preferences prefs("../mec-api/tests/kontrol.json");
    assert(prefs.valid());
    mec::Preferences mec_prefs(prefs.getSubTree("mec"));
    assert(mec_prefs.valid());

    std::shared_ptr<Kontrol::ParameterModel> param_model = Kontrol::ParameterModel::model();
    LOG_1("default values");
    param_model->dumpCurrentValues();

    param_model->applyPreset("0");
    LOG_1("preset 0: transpose 12, rmix 10");
    param_model->dumpCurrentValues();

    param_model->applyPreset("1");
    LOG_1("preset 1: transpose 24");
    param_model->dumpCurrentValues();

    LOG_1("cc 62, 80 : rmix 50");
    param_model->changeMidiCC(1, 16); // do nothing, check doesnt crash ;)
    param_model->changeMidiCC(62, 64);
    param_model->dumpCurrentValues();

    LOG_0("test completed");
    return 0;
}