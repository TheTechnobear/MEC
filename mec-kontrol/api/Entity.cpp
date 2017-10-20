#include "Entity.h"


namespace Kontrol {

// Page
Page::Page(
    const std::string& id,
    const std::string& displayName,
    const std::vector<std::string> paramIds
) : Entity(id, displayName),
    paramIds_(paramIds) {
    ;
}

} //namespace