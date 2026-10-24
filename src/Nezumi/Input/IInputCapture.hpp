// =============================================================================
// IInputCapture.hpp
//
// Something that can claim the mouse or keyboard before the game sees it,
// such as the UI while the cursor is over a panel or a text field has focus.
// The Engine registers the UI with the InputMap (InputMap::set_capture), so
// a click on a button doesn't also fire the game's attack action, and typing
// "w" into a text field doesn't walk the hero north.
//
// Both answers are live (asked at the moment an input event arrives), so the
// cursor's current position decides, not where it was last frame.
// =============================================================================

#pragma once

namespace Nezumi {

class IInputCapture {
public:
    virtual ~IInputCapture() = default;

    // Mouse buttons and the wheel belong to the capturer right now
    virtual bool wants_mouse() const = 0;

    // Keys belong to the capturer right now (a text field has focus)
    virtual bool wants_keyboard() const = 0;
};

} // namespace Nezumi
