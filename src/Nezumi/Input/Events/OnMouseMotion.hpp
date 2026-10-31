// =============================================================================
// OnMouseMotion.hpp
//
// Called when the mouse moved. Positions are window pixels, (0, 0) at the
// top-left, y down: the units of Camera2D::screen_to_world, even on HiDPI
// screens where SDL's window points differ from pixels.
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
