// =============================================================================
// TextureUsage.hpp
//
// WebGPU: GPUTextureUsage
// Vulkan: VkImageUsageFlags
// =============================================================================

#pragma once
#include "Nezumi/Core/EnumClassFlags.hpp"
#include <cstdint> // uint32_t

namespace Nezumi::RHI {

enum class TextureUsageFlags : uint32_t {
    None               = 0,
    Sampled            = 1u << 0, // read in shaders
    ColorTarget        = 1u << 1, // render pass color attachment
    DepthStencilTarget = 1u << 2, // render pass depth attachment
    Storage            = 1u << 3, // read/write in compute
    CopySrc            = 1u << 4,
    CopyDst            = 1u << 5, // required to upload with writeTexture
};
ENUM_CLASS_FLAGS(TextureUsageFlags);

} // namespace Nezumi::RHI
