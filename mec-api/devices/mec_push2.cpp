#include "mec_push2.h"

#include "mec_log.h"
#include "../mec_voice.h"
#include "push2/mec_push2_param.h"
#include "push2/mec_push2_module.h"
#include "push2/mec_push2_preset.h"
#include "push2/mec_push2_play.h"


////////////////////////////////////////////////


namespace mec {

static const unsigned OSC_POLL_MS = 50;


Push2::Push2(ICallback &cb) :
        MidiDevice(cb),
        modulationLearnActive_(false),
        midiLearnActive_(false) {
    PaUtil_InitializeRingBuffer(&midiQueue_, sizeof(MidiMsg), MAX_N_MIDI_MSGS, msgData_);
}

Push2::~Push2() {
    deinit();
}

void Push2InCallback(double deltatime, std::vector<unsigned char> *message, void *userData) {
    Push2 *self = static_cast<Push2 *>(userData);
    self->midiCallback(deltatime, message);
}

void Push2::changeDisplayMode(PushDisplayModes mode) {
    currentDisplayMode_ = mode;
    auto m = displayModes_[mode];
    if (m != nullptr) m->activate();
}

void Push2::addDisplayMode(PushDisplayModes mode, std::shared_ptr<P2_DisplayMode> handler) {
    displayModes_[mode] = handler;
}

void Push2::changePadMode(PushPadModes mode) {
    currentPadMode_ = mode;
    auto m = padModes_[mode];
    if (m != nullptr) m->activate();
}

void Push2::addPadMode(PushPadModes mode, std::shared_ptr<P2_PadMode> handler) {
    padModes_[mode] = handler;
}

RtMidiIn::RtMidiCallback Push2::getMidiCallback() {

    return Push2InCallback;
}

void *push2_processor_func(void *pDevice) {
    Push2 *pThis = static_cast<Push2 *>(pDevice);
    pThis->processorRun();
    return nullptr;
}


bool Push2::init(void *arg) {
    if (MidiDevice::init(arg)) {

        Preferences prefs(arg);

        // push2 api setup
        push2Api_.reset(new Push2API::Push2());
        push2Api_->init();

        push2Api_->clearDisplay();

        // Kontrol setup
        model_ = Kontrol::KontrolModel::model();

        // setup initial modes
        addDisplayMode(P2D_Param, std::make_shared<P2_ParamMode>(*this, push2Api_));
        addDisplayMode(P2D_Module, std::make_shared<P2_ModuleMode>(*this, push2Api_));
        addDisplayMode(P2D_Preset, std::make_shared<P2_PresetMode>(*this, push2Api_));
        changeDisplayMode(P2D_Param);


        addPadMode(P2P_Play, std::make_shared<P2_PlayMode>(*this, push2Api_));
        changePadMode(P2P_Play);

        active_ = true;
        processor_ = std::thread(push2_processor_func, this);
        LOG_0("Push2::init - complete");

        sendCC(0,P2_DEVICE_CC,0x7f);
        sendCC(0,P2_BROWSE_CC,0x7f);
        return active_;
    }
    return false;
}

void Push2::processorRun() {
    while (active_) {
        push2Api_->render();

        while (PaUtil_GetRingBufferReadAvailable(&midiQueue_)) {
            MidiMsg msg;
            PaUtil_ReadRingBuffer(&midiQueue_, &msg, 1);
            processMidi(msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(OSC_POLL_MS));
    }
}

bool Push2::process() {
    //FIXME: consider what thread this needs to be on!
    return MidiDevice::process();
}

void Push2::deinit() {
    LOG_0("Push2::deinit");
    active_ = false;

    if (processor_.joinable()) {
        processor_.join();
    }

    if (push2Api_)push2Api_->deinit();
    push2Api_.reset();
    MidiDevice::deinit();
}

bool Push2::midiCallback(double, std::vector<unsigned char> *message) {
    unsigned int n = message->size();
    if (n > 3) LOG_0("midiCallback unexpect midi size" << n);

    MidiMsg m;

    m.data[0] = message->at(0);
    if (n > 1) m.data[1] = message->at(1);
    if (n > 2) m.data[2] = message->at(2);

    // LOG_0("midi: s " << std::hex << m.status_ << " "<< m.data[1] << " " << m.data[2]);
    PaUtil_WriteRingBuffer(&midiQueue_, (void *) &m, 1);
    return true;
}

bool Push2::processMidi(const MidiMsg &midimsg) {
    //TODO... this can be rationalise with mec_mididevice.cpp
    //be careful : push2 we want to process midi message in main thread
    //since we are going to be handling some of the messages for OLED etc
    int ch = midimsg.data[0] & 0x0F;
    int type = midimsg.data[0] & 0xF0;
    MecMsg msg;
    switch (type) {
        case 0x90: { // note on
            if (midimsg.data[2] > 0) {
                processNoteOn(midimsg.data[1], midimsg.data[2]);
            } else {
                processNoteOff(midimsg.data[1], 100);
            }
            break;
        }

        case 0x80: { // note off
            processNoteOff(midimsg.data[1], midimsg.data[2]);
            break;
        }
        case 0xA0: { // poly pressure
            // TODO: move to play mode
            msg.type_ = MecMsg::TOUCH_CONTINUE;
            msg.data_.touch_.touchId_ = ch;
            msg.data_.touch_.note_ = (float) midimsg.data[1];
            msg.data_.touch_.x_ = 0;
            msg.data_.touch_.y_ = 0;
            msg.data_.touch_.z_ = float(midimsg.data[2]) / 127.0f;
            addTouchMsg(msg);
            break;
        }
        case 0xB0: { // CC
            processCC(midimsg.data[1], midimsg.data[2]);
            break;
        }
        case 0xD0: { // channel pressure
            // float v = float(midimsg.data[1]) / 127.0f;
            // msg.type_ = MecMsg::CONTROL;
            // msg.data_.control_.controlId_ = type;
            // msg.data_.control_.value_ = v;
            // queue_.addToQueue(msg);
            break;
        }
        case 0xE0: { // pitchbend
            // float pb = (float) ((midimsg.data[2] << 7) + midimsg.data[1]);
            // float v = (pb / 8192.0f) - 1.0f;  // -1.0 to 1.0
            // msg.type_ = MecMsg::CONTROL;
            // msg.data_.control_.controlId_ = type;
            // msg.data_.control_.value_ = v;
            // queue_.addToQueue(msg);
            break;
        }
        default: {
            // TODO
            // everything else;
            // need to consider sysex replies?
            break;
        }
    } //switch

    return true;
}


void Push2::processNoteOn(unsigned n, unsigned v) {
    currentDisplayMode()->processNoteOn(n, v);
    currentPadMode()->processNoteOn(n, v);

}

void Push2::processNoteOff(unsigned n, unsigned v) {
    currentDisplayMode()->processNoteOff(n, v);
    currentPadMode()->processNoteOff(n, v);
}


void Push2::processCC(unsigned cc, unsigned v) {
    switch(cc) {
        case P2_DEVICE_CC: {
            if(v>0) {
                if(currentDisplayMode_!=P2D_Module) {
                    changeDisplayMode(P2D_Module);
                } else {
                    changeDisplayMode(P2D_Param);
                }
            }
            break;
        }
        case P2_BROWSE_CC : {
            if(v>0) {
                if(currentDisplayMode_!=P2D_Preset) {
                    changeDisplayMode(P2D_Preset);
                } else {
                    changeDisplayMode(P2D_Param);
                }
            }
            break;
        }
        default: {
            currentDisplayMode()->processCC(cc, v);
            currentPadMode()->processCC(cc, v);
        }
    }
}


void Push2::rack(Kontrol::ChangeSource source, const Kontrol::Rack &rack) {
    if (currentDisplayMode()) currentDisplayMode()->rack(source, rack);
    if (currentPadMode()) currentPadMode()->rack(source, rack);
}

void Push2::module(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module) {
    if (currentDisplayMode()) currentDisplayMode()->module(source, rack, module);
    if (currentPadMode()) currentPadMode()->module(source, rack, module);
}

void Push2::page(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                 const Kontrol::Page &page) {
    if (currentDisplayMode()) currentDisplayMode()->page(source, rack, module, page);
    if (currentPadMode()) currentPadMode()->page(source, rack, module, page);
}

void Push2::param(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                  const Kontrol::Parameter &parameter) {
    if (currentDisplayMode()) currentDisplayMode()->param(source, rack, module, parameter);
    if (currentPadMode()) currentPadMode()->param(source, rack, module, parameter);
}

void Push2::changed(Kontrol::ChangeSource source, const Kontrol::Rack &rack, const Kontrol::Module &module,
                    const Kontrol::Parameter &parameter) {
    if (currentDisplayMode()) currentDisplayMode()->changed(source, rack, module, parameter);
    if (currentPadMode()) currentPadMode()->changed(source, rack, module, parameter);
}


void Push2::resource(Kontrol::ChangeSource source, const Kontrol::Rack &rack,
                     const std::string& resType, const std::string &resValue) {
    if (currentDisplayMode()) currentDisplayMode()->resource(source, rack, resType, resValue);
    if (currentPadMode()) currentPadMode()->resource(source, rack, resType, resValue);
}


void Push2::applyPreset(Kontrol::ChangeSource source, const Kontrol::Rack &rack, std::string preset) {
    if (currentDisplayMode()) currentDisplayMode()->applyPreset(source, rack, preset);
    if (currentPadMode()) currentPadMode()->applyPreset(source, rack, preset);
}

void Push2::midiLearn(Kontrol::ChangeSource src, bool b) {
    if(b) modulationLearnActive_ = false;
    midiLearnActive_ = b;
    if (currentDisplayMode()) currentDisplayMode()->midiLearn(src, b);
    if (currentPadMode()) currentPadMode()->midiLearn(src, b);
}

void Push2::modulationLearn(Kontrol::ChangeSource src, bool b) {
    if(b) midiLearnActive_ = false;
    modulationLearnActive_ = b;
    if (currentDisplayMode()) currentDisplayMode()->modulationLearn(src, b);
    if (currentPadMode()) currentPadMode()->modulationLearn(src, b);
}

void Push2::midiLearn(bool b) {
    model_->midiLearn(Kontrol::CS_LOCAL, b);
}

void Push2::modulationLearn(bool b) {
    model_->modulationLearn(Kontrol::CS_LOCAL, b);
}

void Push2::currentModule(const Kontrol::EntityId id) {
    moduleId_ = id;
    model_->activeModule(Kontrol::CS_LOCAL, rackId_, moduleId_);
}


} // namespace



