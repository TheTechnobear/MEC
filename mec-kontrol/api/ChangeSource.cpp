#include "ChangeSource.h"

namespace Kontrol {

ChangeSource::SrcId ChangeSource::nullId="";

int operator==(const ChangeSource &a, const ChangeSource &b) {
    if (a.type_ == b.type_) {
        // we only use id_ for remote sources
        return a.type_ != ChangeSource::SrcType::REMOTE
               || a.id_ == b.id_;
    }
    return 0;
}

int operator!=(const ChangeSource &a, const ChangeSource &b) {
    return !(a == b);
}


ChangeSource::ChangeSource(SrcType t, SrcId id) : type_(t), id_(id) {
    ;
}


ChangeSource ChangeSource::createRemoteSource(const std::string& host, int port) {
    std::string id = host + ":" + std::to_string(port);
    return ChangeSource(ChangeSource::SrcType::REMOTE, id);

}


}
