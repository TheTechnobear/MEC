#pragma once

#include <vector>
#include <string>

namespace Kontrol {

enum ParameterSource {
    PS_LOCAL,
    PS_MIDI,
    PS_PRESET,
    PS_OSC
};

class Entity {
public:
    Entity(const std::string& id, const std::string& displayName)
        : id_(id), displayName_(displayName) {
        ;
    }

    const std::string& id() const { return id_;};

    virtual const std::string& displayName() const { return displayName_;};
    virtual bool valid() { return !id_.empty();}
protected:
    Entity() {;}
    virtual ~Entity() {;}
    std::string id_;
    std::string displayName_;
};

class Device : public Entity {
public:
    Device(const std::string& id, const std::string& displayName)
        : Entity(id, displayName) {
        ;
    }
private:
};

class Patch : public Entity {
public:
    Patch(const std::string& id, const std::string& displayName)
        : Entity(id, displayName) {
        ;
    }
private:
};


class Page : public Entity {
public:
    Page(
        const std::string& id,
        const std::string& displayName,
        const std::vector<std::string> paramIds
    );
    const std::vector<std::string>& paramIds() const { return paramIds_;}

private:
    std::vector<std::string> paramIds_;
};


} //namespace


