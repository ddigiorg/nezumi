// =============================================================================
// CameraControllerConfig.hpp
//
// Config of a camera controller.
//
// Speeds are in screen pixels: Camera::translate converts them to world units
// (dividing by pixels_per_unit * zoom), so panning feels the same at every zoom
// level and for any world unit size.
//
// Zoom values are Camera zoom factors, relative to the camera's
// pixels_per_unit: with 32 px per tile, zoom_min = 0.25 means 8 px tiles.
//
// Key pan reads four game-defined actions from the InputMap. Left unset (the
// default), keys don't pan the camera.
//
// Follow values are in world units (tiles for the world camera).
// =============================================================================

#pragma once
#include "Nezumi/Input/ActionID.hpp"
#include "Nezumi/Input/Mouse.hpp"
#include <glm/glm.hpp> // vec2

namespace Nezumi {

struct CameraControllerConfig {
    bool        enabled           = true;
    bool        enable_key_pan    = true;  // Pan while a pan action is held
    bool        enable_edge_pan   = false; // Pan while mouse near a window edge
    bool        enable_drag_pan   = true;  // Pan while drag_button held and dragged
    bool        enable_wheel_zoom = true;  // Zoom on mouse wheel scroll
    bool        zoom_to_cursor    = true;  // Keep the world point under the cursor fixed while zooming
    MouseButton drag_button       = MouseButton::Middle;
    float       pan_speed         = 800.0f; // Screen pixels per second (keys and edge)
    float       zoom_step         = 1.15f;  // Zoom multiplier per wheel notch (> 1)
    float       zoom_init         = 1.0f;   // Initial zoom factor
    float       zoom_min          = 0.25f;  // Minimum zoom factor (> 0)
    float       zoom_max          = 4.0f;   // Maximum zoom factor (>= zoom_min)
    float       edge_pixels       = 8.0f;   // Pixels from a window edge that trigger edge pan

    // Key pan actions (unset: no key pan)
    ActionID    pan_left          = {};
    ActionID    pan_right         = {};
    ActionID    pan_down          = {};
    ActionID    pan_up            = {};

    // Follow (see CameraController::set_follow_target)
    float       follow_rate       = 8.0f;          // Catch-up rate per second; 0 snaps every frame
    glm::vec2   follow_dead_zone  = { 0.0f, 0.0f }; // Half size of the box the target moves in freely
};

} // namespace Nezumi
