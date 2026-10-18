// =============================================================================
// TextureConfig.hpp
//
// Config of a Texture.
//
// WebGPU: GPUTextureDescriptor
// Vulkan: VkImageCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/Common/Format.hpp"
#include "Nezumi/RHI/Texture/TextureType.hpp"
#include "Nezumi/RHI/Texture/TextureUsageFlags.hpp"
#include <cstddef>     // byte
#include <cstdint>     // uint32_t
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::RHI {

struct TextureConfig {
    std::string_view  name            = "";
    TextureType       type            = TextureType::Texture2D;
    Format            format          = Format::Undefined;
    std::uint32_t     width           = 0;
    std::uint32_t     height          = 0;
    std::uint32_t     depth_or_layers = 1;   // depth for 3D, layers for arrays, 6 for cube
    std::uint32_t     mip_levels      = 1;
    TextureUsageFlags usage           = TextureUsageFlags::Sampled | TextureUsageFlags::CopyDst;

    // Optional. Mip 0 of every layer, tightly packed, layer after layer.
    std::span<const std::byte> initial_data = {};
};

} // namespace Nezumi::RHI
