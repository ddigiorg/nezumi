// =============================================================================
// CameraControllerConfig.hpp
//
// Config of a camera controller.
//
// Speeds are in screen pixels: Camera::translate divides by zoom, so panning
// feels the same at every zoom level.
// =============================================================================

#pragma once
#include "Nezumi/Input/Mouse.hpp"

namespace Nezumi {

struct CameraControllerConfig {
    bool        enabled           = true;
    bool        enable_key_pan    = true;  // Pan while W/A/S/D or arrow keys held
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
};

} // namespace Nezumi
