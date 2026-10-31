// =============================================================================
// CameraConfig.hpp
//
// Config of a camera.
// =============================================================================

#pragma once
#include "Nezumi/Render/2D/Camera/CameraOrigin.hpp"
#include <glm/glm.hpp> // vec2
#include <string_view> // string_view

namespace Nezumi::Render {

struct CameraConfig {
    std::string_view name     = {};
    CameraOrigin     origin   = CameraOrigin::Centered;
    glm::vec2        position = {   0.0f,   0.0f }; // world-space
    glm::vec2        size     = { 800.0f, 600.0f }; // viewport size in pixels

    // Screen pixels one world unit covers at zoom 1.0. Zoom scales on top of
    // it, so zoom stays a plain "1.0 = default view" factor whatever the
    // world's units are. For a tile-space world drawn at 32 px per tile:
    //
    //   CameraConfig{ .pixels_per_unit = 32.0f }  // zoom 2.0 -> 64 px tiles
    //
    // Leave it at 1.0 for pixel-space cameras (UI, the renderer example).
    float            pixels_per_unit = 1.0f;
};

} // namespace Nezumi::Render
