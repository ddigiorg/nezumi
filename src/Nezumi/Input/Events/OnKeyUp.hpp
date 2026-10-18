// =============================================================================
// OnKeyUp.hpp
//
// Called when a keyboard key was released.
// =============================================================================

#pragma once
#include "Nezumi/Event/IEvent.hpp"
#include "Nezumi/Input/ScanCode.hpp"

namespace Nezumi {

struct OnKeyUp : IEvent {
    ScanCode scancode = ScanCode::Unknown;
    // KeyCode  keycode  = KeyCode::Unknown; // TODO
};

} // namespace Nezumi
