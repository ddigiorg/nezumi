// =============================================================================
// CameraController.hpp
//
// Moves a Render::Camera from user input: key pan, edge pan, mouse drag pan and
// mouse wheel zoom (optionally toward the cursor).
//
// Input events only record state (held keys, cursor position, drag button).
// Camera movement that depends on time happens in update(), which the game
// calls once per frame with the real frame delta:
//
//   void on_draw(Engine& engine) override {
//       camera_controller_->update(engine.clock().get_real_dt());
//       ...
//   }
//
// The camera is presentation, not simulation, so it's updated per frame rather
// than per fixed tick. That keeps panning smooth on displays faster than the
// tick rate.
//
// The camera is referenced by handle and looked up on every use, so removing
// it from the renderer is safe: the controller becomes a no-op.
//
// Window resizes need no handling here: the renderer sizes every view's camera
// to the backbuffer each frame.
// =============================================================================

#pragma once
#include "Nezumi/Control/CameraControllerConfig.hpp"
#include "Nezumi/Event/EventListenerID.hpp"
#include "Nezumi/Object/Handle.hpp"
#include <array>       // array
#include <cstdint>     // uint8_t
#include <glm/glm.hpp> // vec2

namespace Nezumi {

class EventBus;

namespace Render {
    class Camera;
    class Renderer;
}

class CameraController {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    CameraController(
        EventBus&                     events,
        Render::Renderer&             renderer,
        Handle<Render::Camera>        camera,
        const CameraControllerConfig& config = {}
    );

    ~CameraController();

    // Non-copyable and non-movable: listeners capture `this`
    CameraController(const CameraController&)            = delete;
    CameraController& operator=(const CameraController&) = delete;
    CameraController(CameraController&&)                 = delete;
    CameraController& operator=(CameraController&&)      = delete;

    // -------------------------------------------------------------------------
    // Update
    //
    // Applies key and edge pan. dt is clamped so a hitch doesn't fling the
    // camera across the world.
    // -------------------------------------------------------------------------

    void update(float dt);

    // -------------------------------------------------------------------------
    // Setters
    // -------------------------------------------------------------------------

    void set_camera(Handle<Render::Camera> camera) { camera_ = camera; }

    void set_enabled          (bool set);
    void set_enable_key_pan   (bool set) { config_.enable_key_pan    = set; }
    void set_enable_edge_pan  (bool set) { config_.enable_edge_pan   = set; }
    void set_enable_drag_pan  (bool set);
    void set_enable_wheel_zoom(bool set) { config_.enable_wheel_zoom = set; }

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    Handle<Render::Camera>        get_camera () const { return camera_;         }
    const CameraControllerConfig& get_config () const { return config_;         }
    bool                          is_enabled () const { return config_.enabled; }
    bool                          is_dragging() const { return is_dragging_;    }

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    Render::Camera* get_camera_() const;

    void subscribe_();
    void zoom_(float notches);

    glm::vec2 key_dir_ () const;
    glm::vec2 edge_dir_(const Render::Camera& camera) const;

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    // Held movement keys, one bit per direction (see HeldKey_ in the .cpp).
    // Set on key down, cleared on key up, so the state can't drift the way an
    // accumulated +1/-1 direction does when a press or release is missed.
    std::uint8_t held_keys_ = 0u;

    glm::vec2 cursor_      = { 0.0f, 0.0f }; // window pixels, y down
    bool      has_cursor_  = false;          // false until the first mouse motion
    bool      is_dragging_ = false;

    CameraControllerConfig config_;
    Handle<Render::Camera> camera_ = {};

    EventBus&         events_;
    Render::Renderer& renderer_;

    std::array<EventListenerID, 6> listeners_ = {};
};

} // namespace Nezumi
