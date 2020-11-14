#include "e1_basemode.h"

namespace mec {



//{
//p.overlays = std::make_shared < std::vector<ElectraOnePreset::Overlay>>();
//
//ElectraOnePreset::Overlay e;
//e.id = 1;
//ElectraOnePreset::OverlayItem items[2];
//items[0].value = 1;
//items[0].label = "OFF";
//items[1].value = 127;
//items[1].label = "ON";
//e.items.push_back(items[0]);
//e.items.push_back(items[1]);
//p.overlays->push_back(e);
//}

ElectraOneBaseMode::ElectraOneBaseMode(ElectraOne &p) : parent_(p), popupTime_(-1) {
    initPreset();
}

void ElectraOneBaseMode::createParam(unsigned pageid,
                                     unsigned id,
                                     const std::string &name,
                                     int initval,
                                     int min,
                                     int max) {
    unsigned row = id / 2;
    unsigned col = (pageid * 2) + id % 2;
    unsigned potid = row * 6 + col + 1;

    unsigned x = 0 + (170 * col);
    unsigned y = 40 + (88 * row);
    unsigned w = 146;
    unsigned h = 56;

    ElectraOnePreset::Control e;
    e.id = (pageid * 4) + id + 1;
    e.name = std::make_shared<std::string>(name);
    e.page_id = 1;
    e.type = ElectraOnePreset::ControlType::Fader;
    e.bounds.push_back(x);
    e.bounds.push_back(y);
    e.bounds.push_back(w);
    e.bounds.push_back(h);

    e.color = std::make_shared<ElectraOnePreset::Color>(
            ElectraLite::ElectraDevice::getColour(ElectraLite::ElectraDevice::E_RED));
    e.control_set_id = 1;
    e.page_id = 1;

    // e.mode = std::make_shared<ElectraOnePreset::PadMode>(ElectraOnePreset::PadMode::Momentary);

    e.inputs = std::make_shared<std::vector<ElectraOnePreset::Input>>();
    ElectraOnePreset::Input inp;
    inp.pot_id = potid;
    inp.value_id = ElectraOnePreset::ValueId::Value;
    e.inputs->push_back(inp);

    //setup value, cc=encoder id
    ElectraOnePreset::Value val;
    val.id = std::make_shared<ElectraOnePreset::ValueId>(ElectraOnePreset::ValueId::Value);
    val.min = std::make_shared<int64_t>(min);
    val.max = std::make_shared<int64_t>(max);
    val.default_value = std::make_shared<int64_t>(initval);
    // val.overlay_id = std::make_shared<int64_t>(0); // null
    auto &m = val.message;
    m.device_id = 1;
    m.type = ElectraOnePreset::MidiMsgType::Cc7;
    m.parameter_number = std::make_shared<int64_t>(potid);
//    m.off_value = std::make_shared<int64_t>(0);
//    m.on_value = std::make_shared<int64_t>(127);
    m.min = std::make_shared<int64_t>(min);
    m.max = std::make_shared<int64_t>(max);
    e.values.push_back(val);

    auto &p = preset_;
    auto iter = p.controls->begin();
    for (; iter != p.controls->end(); iter++) {
        if (iter->id == e.id) break;
    }
    if (iter == p.controls->end()) p.controls->push_back(e);
    else *iter = e;
}


void ElectraOneBaseMode::createButton(unsigned id, unsigned r,unsigned c, const std::string& name) {
    unsigned row = r + 2;
    unsigned col = c;
    unsigned x = 0 + (170 * col);
    unsigned y = 40 + (88 * row);
    unsigned w = 146;
    unsigned h = 56;

    ElectraOnePreset::Control e;
    e.id = id;
    e.name = std::make_shared<std::string>(name);
    e.page_id = 1;
    e.type = ElectraOnePreset::ControlType::Pad;
    e.mode = std::make_shared<ElectraOnePreset::PadMode>(ElectraOnePreset::PadMode::Momentary);
    e.bounds.push_back(x);
    e.bounds.push_back(y);
    e.bounds.push_back(w);
    e.bounds.push_back(h);

    e.color = std::make_shared<ElectraOnePreset::Color>(
            ElectraLite::ElectraDevice::getColour(ElectraLite::ElectraDevice::E_WHITE));
    e.control_set_id = 1;
    e.page_id = 1;


//    e.inputs = std::make_shared<std::vector<ElectraOnePreset::Input>>();
//    ElectraOnePreset::Input inp;
//    inp.pot_id =e.id;
//    inp.value_id = ElectraOnePreset::ValueId::Value;
//    e.inputs->push_back(inp);

    ElectraOnePreset::Value val;
    val.id = std::make_shared<ElectraOnePreset::ValueId>(ElectraOnePreset::ValueId::Value);
    val.min = std::make_shared<int64_t>(0);
    val.max = std::make_shared<int64_t>(127);
    val.default_value = std::make_shared<int64_t>(0);
    // val.overlay_id = std::make_shared<int64_t>(0); // null
    auto &m = val.message;
    m.device_id = 1;
    m.type = ElectraOnePreset::MidiMsgType::Cc7;
    m.parameter_number = std::make_shared<int64_t>(e.id);
    m.off_value = std::make_shared<int64_t>(0);
    m.on_value = std::make_shared<int64_t>(127);
    m.min = std::make_shared<int64_t>(0);
    m.max = std::make_shared<int64_t>(127);
    e.values.push_back(val);

    auto &p = preset_;
    auto iter = p.controls->begin();
    for (; iter != p.controls->end(); iter++) {
        if (iter->id == e.id) break;
    }
    if (iter == p.controls->end()) p.controls->push_back(e);
    else *iter = e;
}



void ElectraOneBaseMode::createDevice(unsigned id, const std::string &name, unsigned ch, unsigned port) {
    ElectraOnePreset::Device e;
    e.id = id;
    e.name = name;
    e.channel = ch;
    e.port = port;

    auto &p = preset_;
    auto iter = p.devices->begin();
    for (; iter != p.devices->end(); iter++) {
        if (iter->id == e.id) break;
    }
    if (iter == p.devices->end()) p.devices->push_back(e);
    else *iter = e;
}

void ElectraOneBaseMode::createPage(unsigned id, const std::string &name) {
    ElectraOnePreset::Page e;
    e.id = id;
    e.name = name;

    auto &p = preset_;
    auto iter = p.pages->begin();
    for (; iter != p.pages->end(); iter++) {
        if (iter->id == e.id) break;
    }
    if (iter == p.pages->end()) p.pages->push_back(e);
    else *iter = e;
}

void ElectraOneBaseMode::createGroup(unsigned id, const std::string &name) {
    ElectraOnePreset::Group e;
    e.page_id = 1;
    e.name = name;
    unsigned x = 0 + (340 * id);
    unsigned y = 0;
    unsigned w = 147 * 2 + 24;
    unsigned h = 35;
    e.bounds.push_back(x);
    e.bounds.push_back(y);
    e.bounds.push_back(w);
    e.bounds.push_back(h);

    e.color = std::make_shared<ElectraOnePreset::Color>(
            ElectraLite::ElectraDevice::getColour(ElectraLite::ElectraDevice::E_WHITE)
    );

    auto &p = preset_;
    auto iter = p.groups->begin();
    for (; iter != p.groups->end(); iter++) {
        if (iter->name == e.name) break;
    }
    if (iter == p.groups->end()) p.groups->push_back(e);
    else *iter = e;
}

void ElectraOneBaseMode::createKey(unsigned id, const std::string& name, unsigned x,unsigned y) {
    unsigned w = 70;
    unsigned h = 100;

    ElectraOnePreset::Control e;
    e.id = id;
    e.name = std::make_shared<std::string>(name);
    e.page_id = 1;
    e.type = ElectraOnePreset::ControlType::Pad;
    e.mode = std::make_shared<ElectraOnePreset::PadMode>(ElectraOnePreset::PadMode::Momentary);
    e.bounds.push_back(x);
    e.bounds.push_back(y);
    e.bounds.push_back(w);
    e.bounds.push_back(h);

    e.color = std::make_shared<ElectraOnePreset::Color>(
            ElectraLite::ElectraDevice::getColour(ElectraLite::ElectraDevice::E_WHITE));
    e.control_set_id = 1;
    e.page_id = 1;


//    e.inputs = std::make_shared<std::vector<ElectraOnePreset::Input>>();
//    ElectraOnePreset::Input inp;
//    inp.pot_id =e.id;
//    inp.value_id = ElectraOnePreset::ValueId::Value;
//    e.inputs->push_back(inp);

    ElectraOnePreset::Value val;
    val.id = std::make_shared<ElectraOnePreset::ValueId>(ElectraOnePreset::ValueId::Value);
    val.min = std::make_shared<int64_t>(0);
    val.max = std::make_shared<int64_t>(127);
    val.default_value = std::make_shared<int64_t>(0);
    // val.overlay_id = std::make_shared<int64_t>(0); // null
    auto &m = val.message;
    m.device_id = 1;
    m.type = ElectraOnePreset::MidiMsgType::Note;
    m.parameter_number = std::make_shared<int64_t>(e.id);
    m.off_value = std::make_shared<int64_t>(0);
    m.on_value = std::make_shared<int64_t>(127);
    m.min = std::make_shared<int64_t>(0);
    m.max = std::make_shared<int64_t>(127);
    e.values.push_back(val);

    auto &p = preset_;
    auto iter = p.controls->begin();
    for (; iter != p.controls->end(); iter++) {
    if (iter->id == e.id) break;
    }
    if (iter == p.controls->end()) p.controls->push_back(e);
    else *iter = e;
}

void ElectraOneBaseMode::createKeyboard() {
    createKey(50,"Aux",0,450);

    unsigned xi=0;
    unsigned xs=75;
    unsigned xo=200;
    unsigned y=450;

    createKey(60,"C",xi*xs+xo,y);xi++;
    createKey(62,"D",xi*xs+xo,y);xi++;
    createKey(64,"E",xi*xs+xo,y);xi++;
    createKey(65,"F",xi*xs+xo,y);xi++;
    createKey(67,"G",xi*xs+xo,y);xi++;
    createKey(69,"A",xi*xs+xo,y);xi++;
    createKey(71,"B",xi*xs+xo,y);xi++;
    createKey(72,"C",xi*xs+xo,y);xi++;

    xo=xo+(xs/2);
    y= 340;
    xi=0;
    createKey(61,"C#",xi*xs+xo,y);xi++;
    createKey(63,"D#",xi*xs+xo,y);xi++;
    xi++;
    createKey(66,"F#",xi*xs+xo,y);xi++;
    createKey(68,"G#",xi*xs+xo,y);xi++;
    createKey(70,"A#",xi*xs+xo,y);xi++;
}

void ElectraOneBaseMode::clearPages() {
    auto &p = preset_;
    p.pages = std::make_shared<std::vector<ElectraOnePreset::Page>>();
    p.groups = std::make_shared<std::vector<ElectraOnePreset::Group>>();
    p.devices = std::make_shared<std::vector<ElectraOnePreset::Device>>();
//    p.overlays = std::make_shared < std::vector<ElectraOnePreset::Overlay>>();
    p.controls = std::make_shared<std::vector<ElectraOnePreset::Control>>();
}


void ElectraOneBaseMode::initPreset() {
    auto &p = preset_;
    p.version = 2;
    p.name = "Orac";
    p.project_id = std::make_shared<std::string>("Orac-mec-v1");
    clearPages();

    createPage(1, "Orac");
    createDevice(1, "Orac", 1, 1);
}


void ElectraOneBaseMode::poll() {
    if (popupTime_ < 0) return;
    popupTime_--;
}

}// mec
