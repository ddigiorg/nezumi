// =============================================================================
// Viewport.hpp
//
// The rectangular 2D region of a framebuffer where the normalized device
// coordinates (NDC) are mapped during the rasterization stage of the graphics
// pipeline.
//
// Origin is the top-left of the target on every backend.
//
// WebGPU: Configured directly on a render pass encoder using the setViewport
// Vulkan: VkViewport
// =============================================================================

#pragma once

namespace Nezumi::RHI {

struct Viewport {
    float x         = 0.0f;
    float y         = 0.0f;
    float width     = 0.0f;
    float height    = 0.0f;
    float min_depth = 0.0f;
    float max_depth = 1.0f;
};

} // namespace Nezumi::RHI
