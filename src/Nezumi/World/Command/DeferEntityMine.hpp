// =============================================================================
// DeferEntityMine.hpp
// =============================================================================

#pragma once
#include "Nezumi/World/Entity/EntityID.hpp"
#include <cstdint> // uint16_t

namespace Nezumi {

struct DeferEntityMine {
    EntityHandle target = {};
    EntityHandle source = {};
    uint16_t     amount = 0;
};

} // namespace Nezumi
