// =============================================================================
// RandomConfig.hpp
//
// Config of the pseudo-random number generator.
// =============================================================================

#pragma once
#include <cstdint> // uint64_t

namespace Nezumi {

struct RandomConfig {
    uint64_t seed = 0;
};

} // namespace Nezumi
