// =============================================================================
// OnMouseWheel.hpp
//
// Called when the mouse wheel scrolled.
// =============================================================================

#pragma once
#include "Nezumi/Event/IEvent.hpp"

namespace Nezumi {

struct OnMouseWheel : IEvent {
    float rel_x;
    float rel_y;

    bool is_left (float rel_x) const { return rel_x < 0.0f; }
    bool is_right(float rel_x) const { return rel_x > 0.0f; }
    bool is_down (float rel_y) const { return rel_y < 0.0f; }
    bool is_up   (float rel_y) const { return rel_y > 0.0f; }
};

} // namespace Nezumi
