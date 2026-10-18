// =============================================================================
// OnWindowResized.hpp
//
// Called when the game window resized.
// =============================================================================

#pragma once
#include "Nezumi/Event/IEvent.hpp"

namespace Nezumi {

struct OnWindowResized : IEvent {
    int width;
    int height;
};

} // namespace Nezumi
