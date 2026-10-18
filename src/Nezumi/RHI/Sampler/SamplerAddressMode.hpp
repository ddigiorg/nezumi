// =============================================================================
// SamplerAddressMode.hpp
//
// Defines how texture coordinates outside the [0.0, 1.0] range are handled
// during image sampling.
//
// WebGPU: GPUAddressMode
// Vulkan: VkSamplerAddressMode
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class SamplerAddressMode : uint8_t {
    ClampToEdge  = 0, // sensible default for sprites; avoids atlas bleed
    Repeat       = 1,
    MirrorRepeat = 2,
};

} // namespace Nezumi::RHI
