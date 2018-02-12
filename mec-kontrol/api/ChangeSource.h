#pragma once

#include <string>

namespace Kontrol {

class ChangeSource {
public:
    typedef std::string SrcId;
    static SrcId nullId;

    enum SrcType {
        LOCAL,
        MIDI,
        PRESET,
        REMOTE
    };

    ChangeSource(SrcType t,SrcId id=nullId);
    friend int operator==(const ChangeSource& a,const ChangeSource& b);

    static ChangeSource createRemoteSource(const std::string& host, int port);

private:
    SrcType type_;
    SrcId   id_;
};

int operator==(const ChangeSource& a,const ChangeSource& b);
int operator!=(const ChangeSource& a,const ChangeSource& b);

const  ChangeSource CS_LOCAL = ChangeSource(ChangeSource::SrcType::LOCAL);
const  ChangeSource CS_MIDI = ChangeSource(ChangeSource::SrcType::MIDI);
const  ChangeSource CS_PRESET = ChangeSource(ChangeSource::SrcType::PRESET);

} // namespace

