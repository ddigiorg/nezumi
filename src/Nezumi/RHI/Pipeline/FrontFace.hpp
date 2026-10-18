// =============================================================================
// FrontFace.hpp
//
// specifies the vertex winding order used to determine whether a triangle face
// is front-facing or back-facing during rasterization.
//
// WebGPU: A string-based configuration type
// Vulkan: VkFrontFace
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class FrontFace : uint8_t {
    CounterClockwise = 0,
    Clockwise        = 1,
};

} // namespace Nezumi::RHI
