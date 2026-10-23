// =============================================================================
// InputMap.hpp
//
// Maps keys and mouse buttons to game-defined actions, so game code asks "is
// MoveUp held?" instead of "is W held?". Bindings are data: a settings screen
// rebinds a key by calling unbind() and bind(), with no code changes.
//
//   InputMap& input = engine.input();
//   input.bind(Action::MoveUp, { ScanCode::W, ScanCode::Up });
//   ...
//   void on_tick(Engine& engine) override {
//       glm::vec2 dir = engine.input().axis2d(
//           Action::MoveLeft, Action::MoveRight, Action::MoveDown, Action::MoveUp);
//       if (engine.input().was_pressed(Action::Attack)) { ... }
//   }
//
// Held state is the live state of the bound inputs: an action is held while
// any of its inputs is down, so rebinding mid-press can't leave it stuck.
//
// Edges (was_pressed, was_released) are meant for on_tick. They accumulate
// from the input events of one or more frames and are cleared after each
// on_tick, so a tick sees every press since the previous tick exactly once:
// a tap shorter than a tick still registers, and a frame that runs two ticks
// doesn't report the same press twice.
//
// Owned by the Engine (Engine::input()). Listens to the raw input events on
// the EventBus, so those stay available for anything that wants them.
// =============================================================================

#pragma once
#include "Nezumi/Event/EventListenerID.hpp"
#include "Nezumi/Input/ActionID.hpp"
#include "Nezumi/Input/InputSource.hpp"
#include "Nezumi/Input/Mouse.hpp"
#include "Nezumi/Input/ScanCode.hpp"
#include <array>            // array
#include <bitset>           // bitset
#include <glm/glm.hpp>      // vec2
#include <initializer_list> // initializer_list
#include <span>             // span
#include <vector>           // vector

namespace Nezumi {

class EventBus;

class InputMap {
public:

    // -------------------------------------------------------------------------
    // Statics
    // -------------------------------------------------------------------------

    // One past the largest scancode / mouse button value tracked
    static constexpr std::size_t SCANCODE_COUNT     = 512; // SDL_SCANCODE_COUNT
    static constexpr std::size_t MOUSE_BUTTON_COUNT = 8;

    struct Binding {
        ActionID    action = {};
        InputSource source = {};
    };

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    explicit InputMap(EventBus& events);
    ~InputMap();

    // Non-copyable and non-movable: listeners capture `this`
    InputMap(const InputMap&)            = delete;
    InputMap& operator=(const InputMap&) = delete;
    InputMap(InputMap&&)                 = delete;
    InputMap& operator=(InputMap&&)      = delete;

    // -------------------------------------------------------------------------
    // Bindings
    //
    // An action can have any number of inputs and an input can drive any
    // number of actions. Binding the same pair twice is a no-op.
    // -------------------------------------------------------------------------

    void bind(ActionID action, InputSource source);
    void bind(ActionID action, ScanCode    key   ) { bind(action, InputSource::key  (key   )); }
    void bind(ActionID action, MouseButton button) { bind(action, InputSource::mouse(button)); }
    void bind(ActionID action, std::initializer_list<ScanCode> keys);

    void unbind(ActionID action, InputSource source); // one input
    void unbind(ActionID action);                     // all of its inputs
    void clear_bindings();

    // Every binding, in the order bound (e.g. for a settings screen)
    std::span<const Binding> bindings() const { return bindings_; }

    // -------------------------------------------------------------------------
    // Actions
    //
    // An invalid or unbound action is never held, pressed or released, so an
    // optional action (a camera pan the game didn't bind) needs no checks.
    // -------------------------------------------------------------------------

    // Any of the action's inputs is down right now
    bool is_held(ActionID action) const;

    // The action went from not held to held (or back) since the last tick
    bool was_pressed (ActionID action) const;
    bool was_released(ActionID action) const;

    // -1, 0 or +1 from a negative and a positive action. Both held cancel out.
    float axis(ActionID negative, ActionID positive) const {
        return (is_held(positive) ? 1.0f : 0.0f) - (is_held(negative) ? 1.0f : 0.0f);
    }

    // Direction from four actions, world convention (+y up). Each component
    // is -1, 0 or +1, so a diagonal has length sqrt(2): normalize it for
    // movement so diagonals aren't faster.
    glm::vec2 axis2d(ActionID left, ActionID right, ActionID down, ActionID up) const {
        return { axis(left, right), axis(down, up) };
    }

    // -------------------------------------------------------------------------
    // Raw Inputs
    // -------------------------------------------------------------------------

    bool is_down(InputSource source) const;
    bool is_down(ScanCode    key   ) const { return is_down(InputSource::key  (key   )); }
    bool is_down(MouseButton button) const { return is_down(InputSource::mouse(button)); }

    // -------------------------------------------------------------------------
    // Engine
    // -------------------------------------------------------------------------

    // Clears pressed/released edges. Called by the Engine after each on_tick.
    void clear_edges();

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    struct ActionState {
        bool pressed  = false;
        bool released = false;
    };

    void subscribe_();

    // A raw input went up or down: update its state and the edges of the
    // actions bound to it
    void set_down_(InputSource source, bool down);

    // State of `action`, growing the state array if needed. Nullptr if the
    // action is invalid.
    ActionState* state_(ActionID action);

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    std::vector<Binding>     bindings_ = {};
    std::vector<ActionState> states_   = {}; // indexed by ActionID value

    std::bitset<SCANCODE_COUNT>     keys_down_    = {};
    std::bitset<MOUSE_BUTTON_COUNT> buttons_down_ = {};

    // Scratch for set_down_: actions bound to the changing input and whether
    // each was held before the change
    struct Change_ { ActionID action; bool was_held; };
    std::vector<Change_> changes_ = {};

    EventBus& events_;
    std::array<EventListenerID, 4> listeners_ = {};
};

} // namespace Nezumi
