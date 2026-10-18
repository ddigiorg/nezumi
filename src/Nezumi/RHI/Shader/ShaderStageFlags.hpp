// =============================================================================
// ShaderStageFlags.hpp
//
// WebGPU: GPUShaderStage
// Vulkan: VkShaderStageFlags
// =============================================================================

#pragma once
#include "Nezumi/Core/EnumClassFlags.hpp"
#include <cstdint> // uint32_t

namespace Nezumi::RHI {

enum class ShaderStageFlags : uint32_t {
    None     = 0,
    Vertex   = 1u << 0,
    Fragment = 1u << 1,
    Compute  = 1u << 2,
};
ENUM_CLASS_FLAGS(ShaderStageFlags);

} // namespace Nezumi::RHI
