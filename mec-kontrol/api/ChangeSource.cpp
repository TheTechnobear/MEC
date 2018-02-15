#include "ChangeSource.h"

namespace Kontrol {

const ChangeSource::SrcId ChangeSource::nullId="";
const ChangeSource CS_LOCAL = ChangeSource(ChangeSource::SrcType::LOCAL);
const ChangeSource CS_MIDI = ChangeSource(ChangeSource::SrcType::MIDI);
const ChangeSource CS_PRESET = ChangeSource(ChangeSource::SrcType::PRESET);

bool operator==(const ChangeSource &a, const ChangeSource &b) {
    if (a.type_ == b.type_) {
        // we only use id_ for remote sources
        return a.type_ != ChangeSource::SrcType::REMOTE
               || a.id_ == b.id_;
    }
    return false;
}

bool operator!=(const ChangeSource &a, const ChangeSource &b) {
    return !(a == b);
}


ChangeSource::ChangeSource(SrcType t, const SrcId& id) : type_(t), id_(id) {
    ;
}


ChangeSource ChangeSource::createRemoteSource(const std::string& host, int port) {
    std::string id = host + ":" + std::to_string(port);
    return ChangeSource(ChangeSource::SrcType::REMOTE, id);

}


}
