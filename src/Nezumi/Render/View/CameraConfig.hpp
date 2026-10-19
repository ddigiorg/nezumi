// =============================================================================
// CameraConfig.hpp
//
// Config of a camera.
// =============================================================================

#pragma once
#include "Nezumi/Render/View/CameraOrigin.hpp"
#include <glm/glm.hpp> // vec2
#include <string_view> // string_view

namespace Nezumi::Render {

struct CameraConfig {
    std::string_view name     = {};
    CameraOrigin     origin   = CameraOrigin::Centered;
    glm::vec2        position = {   0.0f,   0.0f }; // world-space
    glm::vec2        size     = { 800.0f, 600.0f }; // viewport size in pixels
};

} // namespace Nezumi::Render
