// =============================================================================
// Events/OnEntityMined.hpp
//
// Published when a mine command takes from a resource. `amount` is what was
// actually taken, so it is what the miner should be credited with. When
// `remaining` is 0 the resource ran out and is removed at the next command
// phase (OnEntityRemoved follows).
//
// Sim notification. World enqueues it on the EventBus (EventPhase::TickPost)
// and it is dispatched when the game loop flushes that phase. Listeners are
// outside the simulation: read sim state freely, but change it only through
// World commands.
// =============================================================================

#pragma once
#include "Nezumi/Core/Tick.hpp"
#include "Nezumi/Event/IEvent.hpp"
#include "Nezumi/World/Entity/EntityID.hpp"
#include <cstdint> // uint16_t

namespace Nezumi {

struct OnEntityMined : IEvent {
    EntityHandle    target     = {};
    EntityHandle    source     = {}; // may be null or stale
    EntityTypeID    type_id    = {}; // target's type and subtype: what was
    EntitySubtypeID subtype_id = {}; // mined, without a lookup
    uint16_t        amount     = 0;  // amount actually taken
    uint16_t        remaining  = 0;  // target amount after mining
    Tick            tick       = 0;
};

} // namespace Nezumi
