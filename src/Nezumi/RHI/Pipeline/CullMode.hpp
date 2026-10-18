// =============================================================================
// CullMode.hpp
//
// Defines which triangle faces are discarded during the rasterization stage of
// the graphics pipeline to improve performance.
//
// WebGPU: A property in the primitive state of a render pipeline
// Vulkan: VkCullModeFlags
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class CullMode : uint8_t {
    None  = 0, // sensible default for 2D
    Front = 1,
    Back  = 2,
};

} // namespace Nezumi::RHI
