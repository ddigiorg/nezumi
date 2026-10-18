// =============================================================================
// IndexFormat.hpp
//
// The data type and size of the indices used in indexed drawing commands.
//
// WebGPU: GPUIndexFormat
// Vulkan: VkIndexFormat
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class IndexFormat : uint8_t {
    Uint16 = 0, // For meshes with fewer than 65,536 vertices (saves memory and bandwidth).
    Uint32 = 1, // For large complex meshes with more than 65,535 vertices.
};

} // namespace Nezumi::RHI
