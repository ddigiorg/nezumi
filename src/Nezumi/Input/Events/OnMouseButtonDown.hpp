// =============================================================================
// OnMouseButtonDown.hpp
//
// Called when a mouse button was pressed.
// =============================================================================

#pragma once
#include "Nezumi/Event/IEvent.hpp"
#include "Nezumi/Input/Mouse.hpp"

namespace Nezumi {

struct OnMouseButtonDown : IEvent {
    MouseButton button = MouseButton::Unknown;
};

} // namespace Nezumi
