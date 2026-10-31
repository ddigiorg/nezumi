// =============================================================================
// CameraController.hpp
//
// Moves a Render::Camera2D from user input: key pan, edge pan, mouse drag pan and
// mouse wheel zoom (optionally toward the cursor), or follows a target.
//
// Key pan reads the pan actions named in the config from the InputMap, so the
// game decides which keys pan (if any). Mouse input arrives as events, which
// only record state (cursor position, drag button). Camera2D movement that
// depends on time happens in update(), which the game calls once per frame
// with the real frame delta:
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
// Follow: while a follow target is set, the camera eases toward it in update()
// and key, edge and drag pan are ignored (they would fight the follow). Zoom
// still works, about the screen center. The target is a position, not an
// entity, so the controller stays independent of World; the game passes its
// character's position every frame:
//
//   camera_controller_->set_follow_target(character->position.to_vec2());
//   camera_controller_->update(engine.clock().get_real_dt());
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
#include <glm/glm.hpp> // vec2

namespace Nezumi {

class EventBus;
class InputMap;

namespace Render {
    class Camera2D;
    class Renderer2D;
}

class CameraController {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    CameraController(
        EventBus&                     events,
        const InputMap&               input,
        Render::Renderer2D&             renderer,
        Handle<Render::Camera2D>        camera,
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
    // Applies follow, or key and edge pan. dt is clamped so a hitch doesn't
    // fling the camera across the world.
    // -------------------------------------------------------------------------

    void update(float dt);

    // -------------------------------------------------------------------------
    // Setters
    // -------------------------------------------------------------------------

    void set_camera(Handle<Render::Camera2D> camera) { camera_ = camera; }

    void set_enabled          (bool set);
    void set_enable_key_pan   (bool set) { config_.enable_key_pan    = set; }
    void set_enable_edge_pan  (bool set) { config_.enable_edge_pan   = set; }
    void set_enable_drag_pan  (bool set);
    void set_enable_wheel_zoom(bool set) { config_.enable_wheel_zoom = set; }

    // -------------------------------------------------------------------------
    // Follow
    //
    // Set the target every frame before update(). With `snap`, the camera
    // jumps to it instead of easing (e.g. on spawn or teleport).
    // -------------------------------------------------------------------------

    void set_follow_target(glm::vec2 target, bool snap = false);
    void clear_follow_target();

    bool      is_following()      const { return is_following_;  }
    glm::vec2 get_follow_target() const { return follow_target_; }

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    Handle<Render::Camera2D>        get_camera () const { return camera_;         }
    const CameraControllerConfig& get_config () const { return config_;         }
    bool                          is_enabled () const { return config_.enabled; }
    bool                          is_dragging() const { return is_dragging_;    }

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    Render::Camera2D* get_camera_() const;

    void subscribe_();
    void zoom_(float notches);
    void follow_(Render::Camera2D& camera, float dt);

    glm::vec2 key_dir_ () const;
    glm::vec2 edge_dir_(const Render::Camera2D& camera) const;

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    glm::vec2 cursor_      = { 0.0f, 0.0f }; // window pixels, y down
    bool      has_cursor_  = false;          // false until the first mouse motion
    bool      is_dragging_ = false;

    glm::vec2 follow_target_ = { 0.0f, 0.0f }; // world units
    bool      is_following_  = false;

    CameraControllerConfig config_;
    Handle<Render::Camera2D> camera_ = {};

    EventBus&         events_;
    const InputMap&   input_;
    Render::Renderer2D& renderer_;

    std::array<EventListenerID, 4> listeners_ = {};
};

} // namespace Nezumi
