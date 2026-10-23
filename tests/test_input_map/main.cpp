// =============================================================================
// tests/test_input_map/main.cpp
//
// Headless checks for InputMap: held state across several bound keys, press
// and release edges per tick, key repeats, rebinding while held, axes, and
// mouse buttons.
// =============================================================================

#include "Nezumi/Event/EventBus.hpp"
#include "Nezumi/Input/Events/OnKeyDown.hpp"
#include "Nezumi/Input/Events/OnKeyUp.hpp"
#include "Nezumi/Input/Events/OnMouseButtonDown.hpp"
#include "Nezumi/Input/Events/OnMouseButtonUp.hpp"
#include "Nezumi/Input/InputMap.hpp"
#include <cstdio>  // printf
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS

using namespace Nezumi;

// -----------------------------------------------------------------------------
// Harness
// -----------------------------------------------------------------------------

static int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

namespace Action {
    inline constexpr ActionID Left   = 0;
    inline constexpr ActionID Right  = 1;
    inline constexpr ActionID Down   = 2;
    inline constexpr ActionID Up     = 3;
    inline constexpr ActionID Attack = 7; // ids need not be contiguous
}

struct Rig {
    EventBus bus;
    InputMap input{ bus };

    void key_down(ScanCode key, bool repeat = false) { bus.dispatch(OnKeyDown{ .scancode = key, .repeat = repeat }); }
    void key_up  (ScanCode key)                      { bus.dispatch(OnKeyUp  { .scancode = key }); }

    void mouse_down(MouseButton button) { bus.dispatch(OnMouseButtonDown{ .button = button }); }
    void mouse_up  (MouseButton button) { bus.dispatch(OnMouseButtonUp  { .button = button }); }
};

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

static void test_held_with_two_keys() {
    std::printf("held_with_two_keys\n");
    Rig r;
    r.input.bind(Action::Up, { ScanCode::W, ScanCode::Up });

    CHECK(!r.input.is_held(Action::Up));
    r.key_down(ScanCode::W);
    CHECK(r.input.is_held(Action::Up));
    r.key_down(ScanCode::Up);
    r.key_up(ScanCode::W);
    CHECK(r.input.is_held(Action::Up)); // still held through the arrow key
    r.key_up(ScanCode::Up);
    CHECK(!r.input.is_held(Action::Up));
}

static void test_edges_per_tick() {
    std::printf("edges_per_tick\n");
    Rig r;
    r.input.bind(Action::Attack, ScanCode::Space);

    r.key_down(ScanCode::Space);
    CHECK(r.input.was_pressed(Action::Attack));
    CHECK(!r.input.was_released(Action::Attack));

    r.input.clear_edges(); // the engine does this after each on_tick
    CHECK(!r.input.was_pressed(Action::Attack));
    CHECK(r.input.is_held(Action::Attack));

    // A repeat is not a new press
    r.key_down(ScanCode::Space, /*repeat=*/true);
    CHECK(!r.input.was_pressed(Action::Attack));

    // A tap between two ticks reports both edges, and is no longer held
    r.key_up(ScanCode::Space);
    r.key_down(ScanCode::Space);
    r.key_up(ScanCode::Space);
    CHECK(r.input.was_pressed(Action::Attack));
    CHECK(r.input.was_released(Action::Attack));
    CHECK(!r.input.is_held(Action::Attack));
}

static void test_second_key_is_not_a_press() {
    std::printf("second_key_is_not_a_press\n");
    Rig r;
    r.input.bind(Action::Up, { ScanCode::W, ScanCode::Up });

    r.key_down(ScanCode::W);
    r.input.clear_edges();
    r.key_down(ScanCode::Up); // already held through W
    CHECK(!r.input.was_pressed(Action::Up));
    r.key_up(ScanCode::W);    // still held through Up
    CHECK(!r.input.was_released(Action::Up));
}

static void test_rebind_while_held() {
    std::printf("rebind_while_held\n");
    Rig r;
    r.input.bind(Action::Left, ScanCode::A);

    r.key_down(ScanCode::A);
    CHECK(r.input.is_held(Action::Left));

    // Rebinding to J mid-press can't leave Left stuck down
    r.input.unbind(Action::Left);
    r.input.bind(Action::Left, ScanCode::J);
    CHECK(!r.input.is_held(Action::Left));
    r.key_up(ScanCode::A);
    CHECK(!r.input.is_held(Action::Left));

    CHECK(r.input.bindings().size() == 1);
    r.input.bind(Action::Left, ScanCode::J); // duplicate is a no-op
    CHECK(r.input.bindings().size() == 1);
}

static void test_axes() {
    std::printf("axes\n");
    Rig r;
    r.input.bind(Action::Left,  ScanCode::A);
    r.input.bind(Action::Right, ScanCode::D);
    r.input.bind(Action::Down,  ScanCode::S);
    r.input.bind(Action::Up,    ScanCode::W);

    auto dir = [&] { return r.input.axis2d(Action::Left, Action::Right, Action::Down, Action::Up); };

    CHECK(dir() == glm::vec2(0.0f, 0.0f));
    r.key_down(ScanCode::W);
    r.key_down(ScanCode::D);
    CHECK(dir() == glm::vec2(1.0f, 1.0f)); // +y is up
    r.key_down(ScanCode::A);               // left and right cancel
    CHECK(dir() == glm::vec2(0.0f, 1.0f));

    // Unset and unbound actions read as released
    CHECK(r.input.axis(ActionID{}, Action::Attack) == 0.0f);
    CHECK(!r.input.was_pressed(ActionID{}));
    CHECK(!r.input.was_pressed(ActionID{ 500 }));
}

static void test_mouse_buttons() {
    std::printf("mouse_buttons\n");
    Rig r;
    r.input.bind(Action::Attack, MouseButton::Left);
    r.input.bind(Action::Attack, ScanCode::Space);

    r.mouse_down(MouseButton::Left);
    CHECK(r.input.is_held(Action::Attack));
    CHECK(r.input.was_pressed(Action::Attack));
    CHECK(r.input.is_down(MouseButton::Left));
    r.mouse_up(MouseButton::Left);
    CHECK(!r.input.is_held(Action::Attack));
    CHECK(r.input.was_released(Action::Attack));
}

int main() {
    test_held_with_two_keys();
    test_edges_per_tick();
    test_second_key_is_not_a_press();
    test_rebind_while_held();
    test_axes();
    test_mouse_buttons();

    if (g_failures) {
        std::printf("%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("all passed\n");
    return EXIT_SUCCESS;
}
