#include "e1_basemode.h"

namespace mec {


ElectraOneBaseMode::ElectraOneBaseMode(ElectraOne &p) : parent_(p) {
    initPreset();
}

void ElectraOneBaseMode::createParam(unsigned pageid, unsigned ctrlsetid,
                                     unsigned kpageid, unsigned pos,
                                     unsigned pid,
                                     const std::string &name,
                                     int initval,
                                     int min,
                                     int max) {
    unsigned row = ((ctrlsetid - 1) * 2) + (pos / 2);
    unsigned col = ((kpageid - 1) * 2) + pos % 2;
    unsigned potid = ((pos/2) * 6) + col + 1;

    unsigned x = 0 + (170 * col);
    unsigned y = 40 + (88 * row);
    unsigned w = 146;
    unsigned h = 56;

    ElectraOnePreset::Control e;
    e.id = lastId_++;
    e.control_set_id = ctrlsetid;
    e.page_id = pageid;

//    e.id = (gid * 4) + id + 1;
    e.name = std::make_shared<std::string>(name);
    e.type = ElectraOnePreset::ControlType::Fader;
    e.bounds.push_back(x);
    e.bounds.push_back(y);
    e.bounds.push_back(w);
    e.bounds.push_back(h);

    e.color = std::make_shared<ElectraOnePreset::Color>(
            ElectraLite::ElectraDevice::getColour(ElectraLite::ElectraDevice::E_RED));

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
    m.parameter_number = std::make_shared<int64_t>(pid);
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


void ElectraOneBaseMode::createButton(unsigned id, unsigned pageid, unsigned r, unsigned c, const std::string &name) {
    unsigned row = r;
    unsigned col = c;
    unsigned x = 0 + (170 * col);
    unsigned y = 40 + (88 * row);
    unsigned w = 146;
    unsigned h = 56;

    ElectraOnePreset::Control e;
    e.id = lastId_++;
    e.name = std::make_shared<std::string>(name);
    e.page_id = pageid;
    e.type = ElectraOnePreset::ControlType::Pad;
    e.mode = std::make_shared<ElectraOnePreset::PadMode>(ElectraOnePreset::PadMode::Momentary);
    e.bounds.push_back(x);
    e.bounds.push_back(y);
    e.bounds.push_back(w);
    e.bounds.push_back(h);

    e.color = std::make_shared<ElectraOnePreset::Color>(
            ElectraLite::ElectraDevice::getColour(ElectraLite::ElectraDevice::E_WHITE));
    e.control_set_id = 1;


    ElectraOnePreset::Value val;
    val.id = std::make_shared<ElectraOnePreset::ValueId>(ElectraOnePreset::ValueId::Value);
    val.min = std::make_shared<int64_t>(0);
    val.max = std::make_shared<int64_t>(127);
    val.default_value = std::make_shared<int64_t>(0);
    // val.overlay_id = std::make_shared<int64_t>(0); // null
    auto &m = val.message;
    m.device_id = 1;
    m.type = ElectraOnePreset::MidiMsgType::Note;
    m.parameter_number = std::make_shared<int64_t>(id);
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


void ElectraOneBaseMode::createList(unsigned id, unsigned pageid,
                                    unsigned r,unsigned c,
                                    unsigned pid,
                                    const std::string& name,
                                    std::set<std::string>& list) {

    unsigned row = r;
    unsigned col = c;
    unsigned x = 0 + (170 * col);
    unsigned y = 40 + (88 * row);
//    unsigned w = 146;
    unsigned w = 250;
    unsigned h = 56;

    if(preset_.overlays== nullptr) {
        preset_.overlays = std::make_shared < std::vector<ElectraOnePreset::Overlay>>();
    }

    ElectraOnePreset::Overlay overlay;
    overlay.id = lastOverlayId_;
    unsigned oidx=0;
    for(auto li : list) {
        ElectraOnePreset::OverlayItem item;
        item.value = oidx;
        item.label = li;
        std::replace( item.label.begin(), item.label.end(), '_', ' ');
        oidx++;
        overlay.items.push_back(item);
    }
    preset_.overlays->push_back(overlay);


    ElectraOnePreset::Control e;
    e.id = lastId_++;
    e.name = std::make_shared<std::string>(name);
    e.page_id = pageid;
    e.type = ElectraOnePreset::ControlType::List;
//    e.mode = std::make_shared<ElectraOnePreset::PadMode>(ElectraOnePreset::PadMode::Momentary);
    e.bounds.push_back(x);
    e.bounds.push_back(y);
    e.bounds.push_back(w);
    e.bounds.push_back(h);

    e.color = std::make_shared<ElectraOnePreset::Color>(
            ElectraLite::ElectraDevice::getColour(ElectraLite::ElectraDevice::E_WHITE));
    e.control_set_id = 1;


    e.inputs = std::make_shared<std::vector<ElectraOnePreset::Input>>();
    ElectraOnePreset::Input inp;
    inp.pot_id =pid;
    inp.value_id = ElectraOnePreset::ValueId::Value;
    e.inputs->push_back(inp);

    ElectraOnePreset::Value val;
    val.id = std::make_shared<ElectraOnePreset::ValueId>(ElectraOnePreset::ValueId::Value);
    val.min = std::make_shared<int64_t>(0);
    val.max = std::make_shared<int64_t>(127);
    val.default_value = std::make_shared<int64_t>(0);

    val.overlay_id = std::make_shared<int64_t>(lastOverlayId_); // null
    auto &m = val.message;
    m.device_id = 1;
    m.type = ElectraOnePreset::MidiMsgType::Cc7;
    m.parameter_number = std::make_shared<int64_t>(id);
    m.off_value = std::make_shared<int64_t>(0);
    m.on_value = std::make_shared<int64_t>(list.size());
    m.min = std::make_shared<int64_t>(0);
    m.max = std::make_shared<int64_t>(list.size());
    e.values.push_back(val);

    auto &p = preset_;
    auto iter = p.controls->begin();
    for (; iter != p.controls->end(); iter++) {
        if (iter->id == e.id) break;
    }
    if (iter == p.controls->end()) p.controls->push_back(e);
    else *iter = e;

    lastOverlayId_++;
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

void ElectraOneBaseMode::createGroup(unsigned pageid, unsigned ctrlsetid, unsigned kpageid, const std::string &name) {
    ElectraOnePreset::Group e;
    e.page_id = pageid;
    e.name = name;
    unsigned row = (ctrlsetid - 1) * 2;
    unsigned x = 0 + (340 * (kpageid - 1));
    unsigned y = (88 * row);
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


void ElectraOneBaseMode::clearPages() {
    auto &p = preset_;
    p.pages = std::make_shared<std::vector<ElectraOnePreset::Page>>();
    p.groups = std::make_shared<std::vector<ElectraOnePreset::Group>>();
    p.devices = std::make_shared<std::vector<ElectraOnePreset::Device>>();
    p.overlays = std::make_shared < std::vector<ElectraOnePreset::Overlay>>();
    p.controls = std::make_shared<std::vector<ElectraOnePreset::Control>>();
    lastId_ = 1;
    lastOverlayId_ = 1;
}


void ElectraOneBaseMode::initPreset() {
    auto &p = preset_;
    p.version = 2;
    p.name = "Orac";
    p.project_id = std::make_shared<std::string>("Orac-mec-v1");
    clearPages();

//    createPage(1, "Orac");
    createDevice(1, "Orac", 1, 1);
}


void ElectraOneBaseMode::poll() {
    ;
}


#if 0
void ElectraOneBaseMode::createKey(unsigned id, unsigned pageid, const std::string &name, unsigned x, unsigned y) {
    unsigned w = 70;
    unsigned h = 100;

    ElectraOnePreset::Control e;
    e.id = lastId_++;
    e.name = std::make_shared<std::string>(name);
    e.page_id = pageid;
    e.type = ElectraOnePreset::ControlType::Pad;
    e.mode = std::make_shared<ElectraOnePreset::PadMode>(ElectraOnePreset::PadMode::Momentary);
    e.bounds.push_back(x);
    e.bounds.push_back(y);
    e.bounds.push_back(w);
    e.bounds.push_back(h);

    e.color = std::make_shared<ElectraOnePreset::Color>(
            ElectraLite::ElectraDevice::getColour(ElectraLite::ElectraDevice::E_WHITE));
    e.control_set_id = 1;



    ElectraOnePreset::Value val;
    val.id = std::make_shared<ElectraOnePreset::ValueId>(ElectraOnePreset::ValueId::Value);
    val.min = std::make_shared<int64_t>(0);
    val.max = std::make_shared<int64_t>(127);
    val.default_value = std::make_shared<int64_t>(0);
    // val.overlay_id = std::make_shared<int64_t>(0); // null
    auto &m = val.message;
    m.device_id = 1;
    m.type = ElectraOnePreset::MidiMsgType::Note;
    m.parameter_number = std::make_shared<int64_t>(id);
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

void ElectraOneBaseMode::createKeyboard(unsigned pageid) {
    createKey(E1_BTN_AUX, pageid, "Aux", 0, 450);

    unsigned xi = 0;
    unsigned xs = 75;
    unsigned xo = 200;
    unsigned y = 450;

    createKey(E1_BTN_KEY_START, pageid, "C", xi * xs + xo, y);
    xi++;
    createKey(62, pageid, "D", xi * xs + xo, y);
    xi++;
    createKey(64, pageid, "E", xi * xs + xo, y);
    xi++;
    createKey(65, pageid, "F", xi * xs + xo, y);
    xi++;
    createKey(67, pageid, "G", xi * xs + xo, y);
    xi++;
    createKey(69, pageid, "A", xi * xs + xo, y);
    xi++;
    createKey(71, pageid, "B", xi * xs + xo, y);
    xi++;
    createKey(E1_BTN_KEY_END, pageid, "C", xi * xs + xo, y);
    xi++;

    xo = xo + (xs / 2);
    y = 340;
    xi = 0;
    createKey(61, pageid, "C#", xi * xs + xo, y);
    xi++;
    createKey(63, pageid, "D#", xi * xs + xo, y);
    xi++;
    xi++;
    createKey(66, pageid, "F#", xi * xs + xo, y);
    xi++;
    createKey(68, pageid, "G#", xi * xs + xo, y);
    xi++;
    createKey(70, pageid, "A#", xi * xs + xo, y);
    xi++;
}
#endif


}// mec
