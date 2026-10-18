// =============================================================================
// SamplerFilterMode.hpp
//
// Defines the magintication and minification modes for samplers.
//
// WebGPU: GPUFilterMode or GPUMipmapFilterMode
// Vulkan: VkFilter
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class SamplerFilterMode : uint8_t {
    Nearest = 0, // pixel art
    Linear  = 1,
};

} // namespace Nezumi::RHI
