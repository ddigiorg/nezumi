// =============================================================================
// OnKeyDown.hpp
//
// Called when a keyboard key was pressed.
// =============================================================================

#pragma once
#include "Nezumi/Event/IEvent.hpp"
#include "Nezumi/Input/ScanCode.hpp"

namespace Nezumi {

struct OnKeyDown : IEvent {
    ScanCode scancode = ScanCode::Unknown;
    // KeyCode  keycode  = KeyCode::Unknown; // TODO
    bool repeat = false;
};

} // namespace Nezumi
