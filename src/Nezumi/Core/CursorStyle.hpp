// =============================================================================
// CursorStyle.hpp
//
// The mouse cursor's look, from the operating system's set of cursors. See
// Window::set_cursor(). The UI picks one each frame from what is under the
// mouse (a hand over a button, an I-beam over a text field, arrows on a
// window edge); games can ask for one too through UI::Context::set_cursor().
//
// SDL Reference: https://wiki.libsdl.org/SDL3/SDL_SystemCursor
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi {

enum class CursorStyle : std::uint8_t {
    Arrow,      // default
    Text,       // I-beam, over editable text
    Hand,       // pointing hand, over something clickable
    Move,       // four arrows, dragging a window
    ResizeEW,   // left-right arrows, a window's left or right edge
    ResizeNS,   // up-down arrows, a window's top or bottom edge
    ResizeNWSE, // top-left / bottom-right corner
    ResizeNESW, // top-right / bottom-left corner
    Crosshair,
    NotAllowed,
    Wait,

    Count
};

} // namespace Nezumi
