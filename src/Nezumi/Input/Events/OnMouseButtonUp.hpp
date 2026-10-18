// =============================================================================
// OnMouseButtonDown.hpp
//
// Called when a mouse button was released.
// =============================================================================

#pragma once
#include "Nezumi/Event/IEvent.hpp"
#include "Nezumi/Input/Mouse.hpp"

namespace Nezumi {

struct OnMouseButtonUp : IEvent {
    MouseButton button = MouseButton::Unknown;
};

} // namespace Nezumi
