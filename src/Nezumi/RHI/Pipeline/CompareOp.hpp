// =============================================================================
// CompareOp.hpp
//
// defines comparison operators used for depth tests, stencil tests, and
// shadow/comparison samplers.
//
// WebGPU: GPUCompareFunction
// Vulkan: VkCompareOp
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class CompareOp : uint8_t {
    Never        = 0,
    Less         = 1,
    Equal        = 2,
    LessEqual    = 3,
    Greater      = 4,
    NotEqual     = 5,
    GreaterEqual = 6,
    Always       = 7,
};

} // namespace Nezumi::RHI
