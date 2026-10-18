// =============================================================================
// MemoryUsageFlags.hpp
//
// Where a buffer's memory lives, which is really a statement about how
// often the CPU writes it.
//
// GpuOnly  - Written rarely (static quad, index buffers). Fastest to read.
//            GL: GL_STATIC_DRAW.  Vulkan: device-local, staged uploads.
//
// CpuToGpu - Rewritten every frame (instance data, per-frame uniforms).
//            GL: GL_DYNAMIC_DRAW. Vulkan: host-visible, written directly.
//
// WebGPU: In GPUBufferUsage and GPUTextureUsage
// Vulkan: VkMemoryPropertyFlags
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class MemoryUsageFlags : uint8_t {
    GpuOnly  = 0,
    CpuToGpu = 1,
};

} // namespace Nezumi::RHI
