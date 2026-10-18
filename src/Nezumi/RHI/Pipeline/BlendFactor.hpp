// =============================================================================
// BlendFactor.hpp
//
// Defines the arithmetic weighting factors used in color blending formulas to
// combine incoming fragment colors with existing framebuffer values.
//
// WebGPU: GPUBlendFactor
// Vulkan: VkBlendFactor
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class BlendFactor : uint8_t {
    Zero                  =  0,
    One                   =  1,
    SrcColor              =  2,
    OneMinusSrcColor      =  3,
    SrcAlpha              =  4,
    OneMinusSrcAlpha      =  5,
    DstColor              =  6,
    OneMinusDstColor      =  7,
    DstAlpha              =  8,
    OneMinusDstAlpha      =  9,
    ConstantColor         = 10,
    OneMinusConstantColor = 11,
    SrcAlphaSaturated     = 12,
};

} // namespace Nezumi::RHI
