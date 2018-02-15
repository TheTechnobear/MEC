#pragma once

#include <string>

namespace Kontrol {

class ChangeSource {
public:
    typedef std::string SrcId;
    static const SrcId nullId;

    enum SrcType {
        LOCAL,
        MIDI,
        PRESET,
        REMOTE
    };

    ChangeSource(SrcType t,const SrcId& id=nullId);
    friend bool operator==(const ChangeSource& a,const ChangeSource& b);

    static ChangeSource createRemoteSource(const std::string& host, int port);

private:
    SrcType type_;
    SrcId   id_;
};

bool operator==(const ChangeSource& a,const ChangeSource& b);
bool operator!=(const ChangeSource &a, const ChangeSource &b);

extern const  ChangeSource CS_LOCAL;
extern const  ChangeSource CS_MIDI;
extern const  ChangeSource CS_PRESET;

} // namespace

