// =============================================================================
// Format.hpp
//
// Defines how pixel and block data is structured and interpreted in memory for
// images and vertex buffers.
//
// - Unsigned Normalized (UNORM) maps unsigned integer ranges to float [0.0, 1.0]
// - Signed Normalized (SNORM) maps signed integer ranges to float [-1.0, 1.0]
// - Floating-Point (SFLOAT)
//
// WebGPU: GPUTextureFormat
// Vulkan: VkFormat
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class Format : uint8_t {
    Undefined = 0,

    // 8-bit normalized
    R8Unorm, // single channel; good for font glyph atlases
    RG8Unorm,
    RGBA8Unorm,
    RGBA8UnormSrgb,
    BGRA8Unorm, // common swapchain format on Vulkan / D3D
    BGRA8UnormSrgb,

    // 16-bit float
    R16Float,
    RG16Float,
    RGBA16Float, // HDR render targets

    // 32-bit float
    R32Float,
    RG32Float,
    RGBA32Float,

    // Depth / stencil
    Depth16Unorm,
    Depth24UnormStencil8, // not universally supported on Vulkan; check caps
    Depth32Float,
    Depth32FloatStencil8,
};

} // namespace Nezumi::RHI
