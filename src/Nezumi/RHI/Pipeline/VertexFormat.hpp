// =============================================================================
// VertexFormat.hpp
//
// Defines how the GPU interprets raw memory buffers and maps them to input
// variables in a vertex shader.
//
// - Floating-Point (Float)
// - Unsigned int (Uint)
// - Signed int (Sint)
// - Unsigned Normalized (Unorm) maps unsigned integer ranges to float [0.0, 1.0]
//
// WebGPU: GPUVertexFormat
// Vulkan: VkFormat
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class VertexFormat : uint8_t {
    Float32   =  0,
    Float32x2 =  1,
    Float32x3 =  2,
    Float32x4 =  3,

    Uint32    =  4,
    Uint32x2  =  5,
    Uint32x3  =  6,
    Uint32x4  =  7,

    Sint32    =  8,
    Sint32x2  =  9,
    Sint32x3  = 10,
    Sint32x4  = 11,

    Unorm8x4  = 12,   // packed RGBA color: 4 bytes per instance instead of 16
    Uint8x4   = 13,
    Unorm16x2 = 14,
    Uint16x2  = 15,
};

} // namespace Nezumi::RHI
