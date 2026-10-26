// =============================================================================
// InputMap.cpp
// =============================================================================

#include "Nezumi/Input/InputMap.hpp"
#include "Nezumi/Event/EventBus.hpp"
#include "Nezumi/Input/Events/OnKeyDown.hpp"
#include "Nezumi/Input/Events/OnKeyUp.hpp"
#include "Nezumi/Input/Events/OnMouseButtonDown.hpp"
#include "Nezumi/Input/Events/OnMouseButtonUp.hpp"
#include <algorithm> // any_of, erase_if, find_if

namespace Nezumi {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

InputMap::InputMap(EventBus& events)
    : events_(events)
{
    subscribe_();
}

InputMap::~InputMap() {
    for (EventListenerID id : listeners_)
        events_.unsubscribe(id);
}

// -----------------------------------------------------------------------------
// Bindings
// -----------------------------------------------------------------------------

void InputMap::bind(ActionID action, InputSource source) {
    if (!action) return;

    const bool exists = std::any_of(bindings_.begin(), bindings_.end(),
        [&](const Binding& b) { return b.action == action && b.source == source; });
    if (exists) return;

    // Binding an input that is already down doesn't press the action: it
    // becomes held, but a press is only reported on the next real key down
    state_(action);
    bindings_.push_back({ action, source });
}

void InputMap::bind(ActionID action, std::initializer_list<ScanCode> keys) {
    for (ScanCode key : keys)
        bind(action, InputSource::key(key));
}

void InputMap::unbind(ActionID action, InputSource source) {
    std::erase_if(bindings_,
        [&](const Binding& b) { return b.action == action && b.source == source; });
}

void InputMap::unbind(ActionID action) {
    std::erase_if(bindings_, [&](const Binding& b) { return b.action == action; });
}

void InputMap::clear_bindings() {
    bindings_.clear();
}

std::optional<InputSource> InputMap::first_binding(ActionID action) const {
    for (const Binding& b : bindings_)
        if (b.action == action) return b.source;
    return std::nullopt;
}

// -----------------------------------------------------------------------------
// Actions
// -----------------------------------------------------------------------------

bool InputMap::is_held(ActionID action) const {
    if (!action) return false;

    return std::any_of(bindings_.begin(), bindings_.end(),
        [&](const Binding& b) { return b.action == action && is_down(b.source); });
}

bool InputMap::was_pressed(ActionID action) const {
    return action && action.value() < states_.size() && states_[action.value()].pressed;
}

bool InputMap::was_released(ActionID action) const {
    return action && action.value() < states_.size() && states_[action.value()].released;
}

// -----------------------------------------------------------------------------
// Raw Inputs
// -----------------------------------------------------------------------------

bool InputMap::is_down(InputSource source) const {
    switch (source.kind) {
        case InputSource::Kind::Key:
            return source.code < SCANCODE_COUNT && keys_down_.test(source.code);
        case InputSource::Kind::MouseButton:
            return source.code < MOUSE_BUTTON_COUNT && buttons_down_.test(source.code);
    }
    return false;
}

// -----------------------------------------------------------------------------
// Engine
// -----------------------------------------------------------------------------

void InputMap::clear_edges() {
    for (ActionState& state : states_)
        state = {};
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void InputMap::subscribe_() {

    // Key repeats are ignored: holding a key is one press. Presses the UI
    // captured never reach actions; releases always do (set_down_ ignores a
    // release of something it never saw go down).
    listeners_[0] = events_.subscribe<OnKeyDown>(
        [this](const OnKeyDown& e) {
            if (e.repeat || is_keyboard_captured()) return;
            set_down_(InputSource::key(e.scancode), true);
        }
    );

    listeners_[1] = events_.subscribe<OnKeyUp>(
        [this](const OnKeyUp& e) {
            set_down_(InputSource::key(e.scancode), false);
        }
    );

    listeners_[2] = events_.subscribe<OnMouseButtonDown>(
        [this](const OnMouseButtonDown& e) {
            if (is_mouse_captured()) return;
            set_down_(InputSource::mouse(e.button), true);
        }
    );

    listeners_[3] = events_.subscribe<OnMouseButtonUp>(
        [this](const OnMouseButtonUp& e) {
            set_down_(InputSource::mouse(e.button), false);
        }
    );
}

void InputMap::set_down_(InputSource source, bool down) {
    // Ignore what can't be tracked, and duplicate or missed events (a key up
    // for a key that was never seen going down)
    const std::size_t limit = source.kind == InputSource::Kind::Key
        ? SCANCODE_COUNT
        : MOUSE_BUTTON_COUNT;
    if (source.code == 0u || source.code >= limit) return;
    if (is_down(source) == down) return;

    // Held state of each action bound to this input, before the change. An
    // action held through another input doesn't get a new edge.
    changes_.clear();
    for (const Binding& b : bindings_)
        if (b.source == source)
            changes_.push_back({ b.action, is_held(b.action) });

    if (source.kind == InputSource::Kind::Key) keys_down_   .set(source.code, down);
    else                                       buttons_down_.set(source.code, down);

    for (const Change_& change : changes_) {
        const bool now = is_held(change.action);
        if (now == change.was_held) continue;

        ActionState* state = state_(change.action);
        if (now) state->pressed  = true;
        else     state->released = true;
    }
}

InputMap::ActionState* InputMap::state_(ActionID action) {
    if (!action) return nullptr;
    if (action.value() >= states_.size())
        states_.resize(action.value() + 1u);
    return &states_[action.value()];
}

} // namespace Nezumi
