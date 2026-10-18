// =============================================================================
// StencilOp.hpp
//
// Stencil comparison function.
//
// WebGPU: A property in GPURenderPipeline
// Vulkan: VkStencilOp
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class StencilOp : uint8_t {
    Keep           = 0,
    Zero           = 1,
    Replace        = 2,
    IncrementClamp = 3,
    DecrementClamp = 4,
    Invert         = 5,
    IncrementWrap  = 6,
    DecrementWrap  = 7,
};

} // namespace Nezumi::RHI
