// =============================================================================
// UnitConfig.hpp
//
// Config of a unit.
// =============================================================================

#pragma once
#include <string_view> // string_view

namespace Nezumi {

struct UnitConfig {
    std::string_view name = {};
    // flags
    // factions
    // collision layers
    // health
    // health_max
    // move_speed
    // collision_box
    // selection_box
    // Animation
};

} // namespace Nezumi
