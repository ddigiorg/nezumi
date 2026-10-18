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
};

} // namespace Nezumi
