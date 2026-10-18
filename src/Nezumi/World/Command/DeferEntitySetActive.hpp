// =============================================================================
// DeferEntitySetActive.hpp
// =============================================================================

#pragma once
#include "Nezumi/World/Entity/EntityID.hpp"

namespace Nezumi {

struct DeferEntitySetActive {
    EntityHandle handle = {};
    bool         active = false;
};

} // namespace Nezumi
