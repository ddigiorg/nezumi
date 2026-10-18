// =============================================================================
// WorldConfig.hpp
//
// Config of a World.
// =============================================================================

#pragma once
#include "Nezumi/Core/Tick.hpp"
#include <cstdint>

namespace Nezumi {

struct WorldConfig {
    // Seed for the simulation RNG. Every lockstep peer must use the same one.
    uint64_t seed = 0u;

    // Empty sectors are swept every N ticks (0 disables sweeping).
    Tick sector_sweep_interval = 600u; // 10 s at 60 UPS
};

} // namespace Nezumi
