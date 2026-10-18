// =============================================================================
// SamplerConfig.hpp
//
// Config of a Sampler.
//
// WebGPU: GPUSamplerDescriptor
// Vulkan: VkSamplerCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/Sampler/SamplerAddressMode.hpp"
#include "Nezumi/RHI/Sampler/SamplerFilterMode.hpp"
#include <string_view> // string_view

namespace Nezumi::RHI {

struct SamplerConfig {
    std::string_view   name       = "";
    SamplerFilterMode  min_filter = SamplerFilterMode::Linear;
    SamplerFilterMode  mag_filter = SamplerFilterMode::Linear; // Nearest for pixel art
    SamplerFilterMode  mip_filter = SamplerFilterMode::Nearest;
    SamplerAddressMode address_u  = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode address_v  = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode address_w  = SamplerAddressMode::ClampToEdge;
    float              min_lod    = 0.0f;
    float              max_lod    = 1000.0f;  // effectively "no clamp"
    float              max_anisotropy = 1.0f; // 1 = off
};

} // namespace Nezumi::RHI
