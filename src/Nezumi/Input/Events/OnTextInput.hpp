// =============================================================================
// OnTextInput.hpp
//
// Called with text the user typed, after the keyboard layout and any input
// method (IME) are applied, so "Shift+a" arrives as "A". Only sent while text
// input is on (see Window::set_text_input), which the UI turns on while a
// text field has focus.
//
// Keys that edit rather than type (Backspace, arrows, Enter) arrive as
// OnKeyDown, not here.
// =============================================================================

#pragma once
#include "Nezumi/Event/IEvent.hpp"

namespace Nezumi {

struct OnTextInput : IEvent {
    // UTF-8, null-terminated. SDL's buffer is only valid during the event,
    // so the text is copied; longer input (rare, from an IME) is cut.
    char text[32] = {};
};

} // namespace Nezumi
