// =============================================================================
// BufferUsageFlags.hpp
//
// A set of configuration flags telling the driver how a specific buffer will be
// used during its lifetime.
//
// WebGPU: GPUBufferUsage
// Vulkan: VkBufferUsageFlags
// =============================================================================

#pragma once
#include "Nezumi/Core/EnumClassFlags.hpp"
#include <cstdint> // uint32_t

namespace Nezumi::RHI {

enum class BufferUsageFlags : uint32_t {
    None     = 0,
    Vertex   = 1u << 0,
    Index    = 1u << 1,
    Uniform  = 1u << 2,
    Storage  = 1u << 3,
    Indirect = 1u << 4,
    CopySrc  = 1u << 5,
    CopyDst  = 1u << 6, // required to upload with writeBuffer
};
ENUM_CLASS_FLAGS(BufferUsageFlags);

} // namespace Nezumi::RHI
