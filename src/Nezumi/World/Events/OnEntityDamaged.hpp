// =============================================================================
// Events/OnEntityDamaged.hpp
//
// Published when a damage command is applied to an entity.
//
// Sim notification. World enqueues it on the EventBus (EventPhase::TickPost)
// and it is dispatched when the game loop flushes that phase. Listeners are
// outside the simulation: read sim state freely, but change it only through
// World commands.
// =============================================================================

// TODO

// #pragma once
// #include "Nezumi/Core/Tick.hpp"
// #include "Nezumi/Event/IEvent.hpp"
// #include "Nezumi/World/Entity/EntityID.hpp"
// #include <cstdint> // uint16_t

// namespace Nezumi {

// struct OnEntityDamaged : IEvent {
//     EntityHandle target = {};
//     EntityHandle source = {}; // may be null or stale
//     uint16_t     amount = 0;  // damage actually applied
//     uint16_t     health = 0;  // target health after damage
//     Tick         tick   = 0;
// };

// } // namespace Nezumi
