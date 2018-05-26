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
        REMOTE,
        MODULATION
    };

    ChangeSource(SrcType t,const SrcId& id=nullId);
    friend bool operator==(const ChangeSource& a,const ChangeSource& b);
    SrcType type() { return type_;}

    static ChangeSource createRemoteSource(const std::string& host, int port);

private:
    SrcType type_;
    SrcId   id_;
};

bool operator==(const ChangeSource& a,const ChangeSource& b);
bool operator!=(const ChangeSource &a, const ChangeSource &b);

#ifdef _MSC_VER
#   ifdef mec_kontrol_api_EXPORTS
#       define MEC_KONTROL_API __declspec(dllexport)
#	else
#       define MEC_KONTROL_API __declspec(dllimport)
#   endif
#else
#   define MEC_KONTROL_API
#endif

extern MEC_KONTROL_API const  ChangeSource CS_LOCAL;
extern MEC_KONTROL_API const  ChangeSource CS_MIDI;
extern MEC_KONTROL_API const  ChangeSource CS_MODULATION;
extern MEC_KONTROL_API const  ChangeSource CS_PRESET;

} // namespace

