#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

namespace mec {
class Preferences;
}

namespace Kontrol {

// Eventually give its own type, but will need to be able to be used in maps, ostream etc.
// class EntityId {
// public:
//     EntityId()  { ; }
//     EntityId(const std::string& id) : id_(id) { ; }
//     EntityId(const EntityId& id) : id_(id.id_) { ; }
//     bool valid() { return !id_.empty();}

//     // operator const char*() { return id_.c_str();}
//     operator std::string() { return id_;}
//     EntityId& operator =(const EntityId& id) { id_=id.id_; return *this;}

// private:
//     std::string id_;
// };

typedef std::string EntityId;

class Entity {
public:
    Entity(const EntityId& id, const std::string& displayName)
        : id_(id), displayName_(displayName) {
        ;
    }

    const EntityId& id() const { return id_;};

    virtual const std::string& displayName() const { return displayName_;};
    virtual bool valid() { return id_.empty();}
protected:
    Entity() {;}
    virtual ~Entity() {;}
    EntityId id_;
    std::string displayName_;
};

class Patch;

class Device : public Entity {
public:
    Device( const std::string& host,
            unsigned port,
            const std::string& displayName)
        : Entity(createId(host, port), displayName) {
        ;
    }

    static EntityId createId(const std::string& host, unsigned port) {
        return (host + ":" + std::to_string(port));
    }

    std::vector<std::shared_ptr<Patch>>  getPatches();
    std::shared_ptr<Patch> getPatch(const EntityId& patchId);

    void addPatch(const std::shared_ptr<Patch>& patch);

private:
    std::string host_;
    unsigned port_;

    std::unordered_map<EntityId,std::shared_ptr<Patch>> patches_;
};


class Page : public Entity {
public:
    Page(
        const EntityId& id,
        const std::string& displayName,
        const std::vector<std::string> paramIds
    ) : Entity(id, displayName),
        paramIds_(paramIds) {
    ;
}
    const std::vector<std::string>& paramIds() const { return paramIds_;}

private:
    std::vector<std::string> paramIds_;
};

} //namespace


