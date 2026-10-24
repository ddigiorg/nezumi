// =============================================================================
// WindowConfig.hpp
//
// Config of a window.
// =============================================================================

#pragma once

namespace Nezumi {

struct WindowConfig {
    const char* title      = "Window";
    int         width      = 800;
    int         height     = 600;
    int         min_width  = 0;
    int         min_height = 0;
    bool        mouse_grab = false;

    // Render at the display's native resolution on HiDPI screens (Wayland
    // and macOS scaling, Windows at more than 100%). Off, the window renders
    // at its size in points and the desktop stretches it, which blurs text
    // and every edge; Window::get_display_scale() is then always 1.
    bool        high_pixel_density = true;
};

} // namespace Nezumi
