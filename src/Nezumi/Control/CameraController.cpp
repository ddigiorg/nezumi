// =============================================================================
// CameraController.cpp
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/Event/EventBus.hpp"
#include "Nezumi/Input/Events/OnMouseButtonDown.hpp"
#include "Nezumi/Input/Events/OnMouseButtonUp.hpp"
#include "Nezumi/Input/Events/OnMouseMotion.hpp"
#include "Nezumi/Input/Events/OnMouseWheel.hpp"
#include "Nezumi/Input/InputMap.hpp"
#include "Nezumi/Render/2D/Renderer2D.hpp"
#include "Nezumi/Render/2D/Camera/Camera2D.hpp"
#include <algorithm> // clamp, max, min
#include <cassert>   // assert
#include <cmath>     // abs, exp, pow

namespace Nezumi {

namespace {

// Longest frame delta update() will apply (seconds)
constexpr float MAX_DT = 0.1f;

// Clamp config values to something the controller can work with
CameraControllerConfig sanitize_(CameraControllerConfig config) {
    assert(config.zoom_min  >  0.0f            && "CameraController: zoom_min must be > 0");
    assert(config.zoom_max  >= config.zoom_min && "CameraController: zoom_max must be >= zoom_min");
    assert(config.zoom_step >  1.0f            && "CameraController: zoom_step must be > 1");
    assert(config.follow_rate >= 0.0f          && "CameraController: follow_rate must be >= 0");

    config.zoom_min  = std::max(config.zoom_min, 1e-4f);
    config.zoom_max  = std::max(config.zoom_max, config.zoom_min);
    config.zoom_init = std::clamp(config.zoom_init, config.zoom_min, config.zoom_max);
    config.follow_rate      = std::max(config.follow_rate, 0.0f);
    config.follow_dead_zone = glm::max(config.follow_dead_zone, glm::vec2(0.0f));
    return config;
}

} // namespace

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

CameraController::CameraController(
    EventBus&                     events,
    const InputMap&               input,
    Render::Renderer2D&             renderer,
    Handle<Render::Camera2D>        camera,
    const CameraControllerConfig& config)
        : config_  (sanitize_(config))
        , camera_  (camera)
        , events_  (events)
        , input_   (input)
        , renderer_(renderer)
{
    if (Render::Camera2D* c = get_camera_())
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

    Render::Camera2D* camera = get_camera_();
    if (!camera) return;

    dt = std::clamp(dt, 0.0f, MAX_DT);

    if (is_following_) {
        follow_(*camera, dt);
        return;
    }

    glm::vec2 dir = { 0.0f, 0.0f };
    if (config_.enable_key_pan ) dir += key_dir_();
    if (config_.enable_edge_pan) dir += edge_dir_(*camera);
    if (dir == glm::vec2(0.0f)) return;

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
// Follow
// -----------------------------------------------------------------------------

void CameraController::set_follow_target(glm::vec2 target, bool snap) {
    follow_target_ = target;
    is_following_  = true;
    is_dragging_   = false; // a drag in progress would fight the follow

    if (!snap) return;
    if (Render::Camera2D* camera = get_camera_())
        camera->set_position(target);
}

void CameraController::clear_follow_target() {
    is_following_ = false;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

Render::Camera2D* CameraController::get_camera_() const {
    // SlotMap handles are versioned: a removed camera resolves to nullptr
    return renderer_.get_camera(camera_);
}

void CameraController::subscribe_() {

    listeners_[0] = events_.subscribe<OnMouseButtonDown>(
        [this](const OnMouseButtonDown& e) {
            if (!config_.enabled || !config_.enable_drag_pan || is_following_) return;
            if (input_.is_mouse_captured()) return; // pressed on the UI
            if (e.button == config_.drag_button) is_dragging_ = true;
        }
    );

    // Released regardless of enabled, so a drag never sticks
    listeners_[1] = events_.subscribe<OnMouseButtonUp>(
        [this](const OnMouseButtonUp& e) {
            if (e.button == config_.drag_button) is_dragging_ = false;
        }
    );

    listeners_[2] = events_.subscribe<OnMouseMotion>(
        [this](const OnMouseMotion& e) {
            cursor_     = { e.pos_x, e.pos_y };
            has_cursor_ = true;

            if (!is_dragging_) return;
            if (Render::Camera2D* camera = get_camera_()) {
                // Screen y points down and world y points up. Moving the
                // camera opposite to the drag keeps the grabbed point under
                // the cursor (translate converts pixels to world units).
                camera->translate({ -e.rel_x, e.rel_y });
            }
        }
    );

    listeners_[3] = events_.subscribe<OnMouseWheel>(
        [this](const OnMouseWheel& e) {
            if (!config_.enabled || !config_.enable_wheel_zoom) return;
            if (input_.is_mouse_captured()) return; // scrolling a UI panel
            if (e.rel_y != 0.0f) zoom_(e.rel_y);
        }
    );
}

void CameraController::zoom_(float notches) {
    Render::Camera2D* camera = get_camera_();
    if (!camera) return;

    const float old_zoom = camera->get_zoom();
    const float new_zoom = std::clamp(
        old_zoom * std::pow(config_.zoom_step, notches),
        config_.zoom_min,
        config_.zoom_max
    );
    if (new_zoom == old_zoom) return;

    // While following, zoom about the screen center: shifting the camera
    // toward the cursor would only be undone by the follow
    if (!config_.zoom_to_cursor || !has_cursor_ || is_following_) {
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

void CameraController::follow_(Render::Camera2D& camera, float dt) {
    const glm::vec2 position = camera.get_position();
    const glm::vec2 offset   = follow_target_ - position;

    // Only the part of the offset outside the dead zone pulls the camera, so
    // the target moves freely inside it and is held at its edge
    const glm::vec2 excess = glm::sign(offset)
                           * glm::max(glm::abs(offset) - config_.follow_dead_zone, 0.0f);
    if (excess == glm::vec2(0.0f)) return;

    // Exponential ease: covers the same fraction of the gap per second at any
    // frame rate. A rate of 0 snaps.
    const float t = config_.follow_rate > 0.0f
        ? 1.0f - std::exp(-config_.follow_rate * dt)
        : 1.0f;

    camera.set_position(position + excess * t);
}

glm::vec2 CameraController::key_dir_() const {
    return input_.axis2d(
        config_.pan_left,
        config_.pan_right,
        config_.pan_down,
        config_.pan_up
    );
}

glm::vec2 CameraController::edge_dir_(const Render::Camera2D& camera) const {
    // No edge pan while the cursor rests on a UI panel at the screen edge
    if (!has_cursor_ || input_.is_mouse_captured()) return { 0.0f, 0.0f };

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
