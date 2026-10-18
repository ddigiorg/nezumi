// =============================================================================
// BlendOp.hpp
//
// Defines the mathematical operation used to combine source (fragment) and
// destination (framebuffer) color or alpha values.
//
// WebGPU: GPUBlendOperation
// Vulkan: VkBlendOp
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class BlendOp : uint8_t {
    Add             = 0,
    Subtract        = 1,
    ReverseSubtract = 2,
    Min             = 3,
    Max             = 4,
};

} // namespace Nezumi::RHI
