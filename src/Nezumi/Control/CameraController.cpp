// =============================================================================
// CameraController.cpp
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/Event/EventBus.hpp"
#include "Nezumi/Input/Events/OnKeyDown.hpp"
#include "Nezumi/Input/Events/OnKeyUp.hpp"
#include "Nezumi/Input/Events/OnMouseButtonDown.hpp"
#include "Nezumi/Input/Events/OnMouseButtonUp.hpp"
#include "Nezumi/Input/Events/OnMouseMotion.hpp"
#include "Nezumi/Input/Events/OnMouseWheel.hpp"
#include "Nezumi/Input/ScanCode.hpp"
#include "Nezumi/Render/Renderer.hpp"
#include "Nezumi/Render/View/Camera.hpp"
#include <algorithm> // clamp, max, min
#include <cassert>   // assert
#include <cmath>     // pow

namespace Nezumi {

namespace {

// Longest frame delta update() will apply (seconds)
constexpr float MAX_DT = 0.1f;

// Bits of CameraController::held_keys_
enum HeldKey_ : std::uint8_t {
    HELD_LEFT  = 1u << 0,
    HELD_RIGHT = 1u << 1,
    HELD_DOWN  = 1u << 2,
    HELD_UP    = 1u << 3,
};

// Movement bit for a scancode, or 0 if the key doesn't move the camera
std::uint8_t held_key_bit_(ScanCode scancode) {
    switch (scancode) {
        case ScanCode::A: case ScanCode::Left:  return HELD_LEFT;
        case ScanCode::D: case ScanCode::Right: return HELD_RIGHT;
        case ScanCode::S: case ScanCode::Down:  return HELD_DOWN;
        case ScanCode::W: case ScanCode::Up:    return HELD_UP;
        default:                                return 0u;
    }
}

// Clamp config values to something the controller can work with
CameraControllerConfig sanitize_(CameraControllerConfig config) {
    assert(config.zoom_min  >  0.0f            && "CameraController: zoom_min must be > 0");
    assert(config.zoom_max  >= config.zoom_min && "CameraController: zoom_max must be >= zoom_min");
    assert(config.zoom_step >  1.0f            && "CameraController: zoom_step must be > 1");

    config.zoom_min  = std::max(config.zoom_min, 1e-4f);
    config.zoom_max  = std::max(config.zoom_max, config.zoom_min);
    config.zoom_init = std::clamp(config.zoom_init, config.zoom_min, config.zoom_max);
    return config;
}

} // namespace

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

CameraController::CameraController(
    EventBus&                     events,
    Render::Renderer&             renderer,
    Handle<Render::Camera>        camera,
    const CameraControllerConfig& config)
        : config_  (sanitize_(config))
        , camera_  (camera)
        , events_  (events)
        , renderer_(renderer)
{
    if (Render::Camera* c = get_camera_())
        c->set_zoom(config_.zoom_init);

    subscribe_();
}

CameraController::~CameraController() {
    for (EventListenerID id : listeners_)
        events_.unsubscribe(id);
}

// -----------------------------------------------------------------------------
// Update
// -----------------------------------------------------------------------------

void CameraController::update(float dt) {
    if (!config_.enabled) return;

    Render::Camera* camera = get_camera_();
    if (!camera) return;

    glm::vec2 dir = { 0.0f, 0.0f };
    if (config_.enable_key_pan ) dir += key_dir_();
    if (config_.enable_edge_pan) dir += edge_dir_(*camera);
    if (dir == glm::vec2(0.0f)) return;

    dt = std::clamp(dt, 0.0f, MAX_DT);

    // Clamp then normalize, so diagonals aren't faster and key + edge pan in
    // the same direction don't stack
    dir = glm::clamp(dir, -1.0f, 1.0f);
    camera->translate(glm::normalize(dir) * config_.pan_speed * dt);
}

// -----------------------------------------------------------------------------
// Setters
// -----------------------------------------------------------------------------

void CameraController::set_enabled(bool set) {
    config_.enabled = set;
    if (!set) is_dragging_ = false;
}

void CameraController::set_enable_drag_pan(bool set) {
    config_.enable_drag_pan = set;
    if (!set) is_dragging_ = false;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

Render::Camera* CameraController::get_camera_() const {
    // SlotMap handles are versioned: a removed camera resolves to nullptr
    return renderer_.get_camera(camera_);
}

void CameraController::subscribe_() {

    // Key state is tracked even while disabled, so re-enabling with a key
    // already held behaves as expected.
    listeners_[0] = events_.subscribe<OnKeyDown>(
        [this](const OnKeyDown& e) {
            held_keys_ |= held_key_bit_(e.scancode);
        }
    );

    listeners_[1] = events_.subscribe<OnKeyUp>(
        [this](const OnKeyUp& e) {
            held_keys_ &= static_cast<std::uint8_t>(~held_key_bit_(e.scancode));
        }
    );

    listeners_[2] = events_.subscribe<OnMouseButtonDown>(
        [this](const OnMouseButtonDown& e) {
            if (!config_.enabled || !config_.enable_drag_pan) return;
            if (e.button == config_.drag_button) is_dragging_ = true;
        }
    );

    // Released regardless of enabled, so a drag never sticks
    listeners_[3] = events_.subscribe<OnMouseButtonUp>(
        [this](const OnMouseButtonUp& e) {
            if (e.button == config_.drag_button) is_dragging_ = false;
        }
    );

    listeners_[4] = events_.subscribe<OnMouseMotion>(
        [this](const OnMouseMotion& e) {
            cursor_     = { e.pos_x, e.pos_y };
            has_cursor_ = true;

            if (!is_dragging_) return;
            if (Render::Camera* camera = get_camera_()) {
                // Screen y points down and world y points up. Moving the
                // camera opposite to the drag keeps the grabbed point under
                // the cursor (translate divides by zoom).
                camera->translate({ -e.rel_x, e.rel_y });
            }
        }
    );

    listeners_[5] = events_.subscribe<OnMouseWheel>(
        [this](const OnMouseWheel& e) {
            if (!config_.enabled || !config_.enable_wheel_zoom) return;
            if (e.rel_y != 0.0f) zoom_(e.rel_y);
        }
    );
}

void CameraController::zoom_(float notches) {
    Render::Camera* camera = get_camera_();
    if (!camera) return;

    const float old_zoom = camera->get_zoom();
    const float new_zoom = std::clamp(
        old_zoom * std::pow(config_.zoom_step, notches),
        config_.zoom_min,
        config_.zoom_max
    );
    if (new_zoom == old_zoom) return;

    if (!config_.zoom_to_cursor || !has_cursor_) {
        camera->set_zoom(new_zoom);
        return;
    }

    // Zoom about the cursor: shift the camera by however far the world point
    // under the cursor moved, so it ends up back under the cursor.
    const glm::vec2 before = camera->screen_to_world(cursor_);
    camera->set_zoom(new_zoom);
    const glm::vec2 after  = camera->screen_to_world(cursor_);
    camera->set_position(camera->get_position() + (before - after));
}

glm::vec2 CameraController::key_dir_() const {
    glm::vec2 dir = { 0.0f, 0.0f };
    if (held_keys_ & HELD_LEFT ) dir.x -= 1.0f;
    if (held_keys_ & HELD_RIGHT) dir.x += 1.0f;
    if (held_keys_ & HELD_DOWN ) dir.y -= 1.0f;
    if (held_keys_ & HELD_UP   ) dir.y += 1.0f;
    return dir;
}

glm::vec2 CameraController::edge_dir_(const Render::Camera& camera) const {
    if (!has_cursor_) return { 0.0f, 0.0f };

    // Cursor is in window pixels with y down; the camera is sized to the
    // backbuffer, which matches the window unless the display is HiDPI.
    const glm::vec2 size = camera.get_size();
    const float     m    = config_.edge_pixels;

    glm::vec2 dir = { 0.0f, 0.0f };
    if (cursor_.x <= m)          dir.x -= 1.0f;
    if (cursor_.x >= size.x - m) dir.x += 1.0f;
    if (cursor_.y >= size.y - m) dir.y -= 1.0f;
    if (cursor_.y <= m)          dir.y += 1.0f;
    return dir;
}

} // namespace Nezumi
