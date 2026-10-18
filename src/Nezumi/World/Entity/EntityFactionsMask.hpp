// =============================================================================
// EntityFactionsMask.hpp
//
// Entity factions.
// =============================================================================

#pragma once
#include "Nezumi/Core/EnumClassFlags.hpp"
#include <cstdint> // uint16_t

namespace Nezumi {

enum class EntityFactionsMask : std::uint16_t {
    None      = 0u,
    Faction00 = 1u <<  0,
    Faction01 = 1u <<  1,
    Faction02 = 1u <<  2,
    Faction03 = 1u <<  3,
    Faction04 = 1u <<  4,
    Faction05 = 1u <<  5,
    Faction06 = 1u <<  6,
    Faction07 = 1u <<  7,
    Faction08 = 1u <<  8,
    Faction09 = 1u <<  9,
    Faction10 = 1u << 10,
    Faction11 = 1u << 11,
    Faction12 = 1u << 12,
    Faction13 = 1u << 13,
    Faction14 = 1u << 14,
    Faction15 = 1u << 15,
};

ENUM_CLASS_FLAGS(EntityFactionsMask);

} // namespace Nezumi
