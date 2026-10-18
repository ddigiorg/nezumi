// =============================================================================
// OnEntityCreated.hpp
//
// Published after a spawned entity is inserted into its surface.
//
// Sim notification. World enqueues it on the EventBus (EventPhase::TickPost)
// and it is dispatched when the game loop flushes that phase. Listeners are
// outside the simulation: read sim state freely, but change it only through
// World commands.
// =============================================================================

#pragma once
#include "Nezumi/Core/Tick.hpp"
#include "Nezumi/Event/IEvent.hpp"
#include "Nezumi/World/Surface/SurfaceID.hpp"
#include "Nezumi/World/Entity/EntityID.hpp"

namespace Nezumi {

struct OnEntityCreated : IEvent {
    EntityHandle handle    = {};
    EntityTypeID type_id   = {};
    SurfaceID     surface_id = {};
    Tick         tick      = 0;
};

} // namespace Nezumi
