// =============================================================================
// OnMouseMotion.hpp
//
// Called when the mouse moved.
// =============================================================================

#pragma once
#include "Nezumi/Event/IEvent.hpp"

namespace Nezumi {

struct OnMouseMotion : IEvent {
    float pos_x = 0.0f; // absolute mouse position x
    float pos_y = 0.0f; // absolute mouse position y
    float rel_x = 0.0f; // relative mouse position x
    float rel_y = 0.0f; // relative mouse position y
};

} // namespace Nezumi
