// =============================================================================
// OnChunkCreated.hpp
//
// Published after a chunk is created (on demand or explicitly).
//
// Sim notification. World enqueues it on the EventBus (EventPhase::TickPost)
// and it is dispatched when the game loop flushes that phase. Listeners are
// outside the simulation: read sim state freely, but change it only through
// World commands.
// =============================================================================

#pragma once
#include "Nezumi/Core/Tick.hpp"
#include "Nezumi/Event/IEvent.hpp"
#include "Nezumi/World/Chunk/ChunkID.hpp"
#include "Nezumi/World/Surface/SurfaceID.hpp"

namespace Nezumi {

struct OnChunkCreated : IEvent {
    SurfaceID surface_id = {};
    ChunkID  chunk_id  = {};
    Tick     tick      = 0;
};

} // namespace Nezumi
