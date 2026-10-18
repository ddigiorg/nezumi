// =============================================================================
// Tick.hpp
//
// The base unit of time in the game simulation loop.
// =============================================================================

#pragma once
#include <cstdint>

namespace Nezumi {

// TODO: consider making strong type instead of alias:
//   enum class Tick : uint64_t {};
using Tick = uint64_t;

} // namespace Nezumi
