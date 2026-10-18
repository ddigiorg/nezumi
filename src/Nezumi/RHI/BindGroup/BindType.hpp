// =============================================================================
// BindType.hpp
//
// The binding type of a resource within a BindGroupLayout.
//
// Textures and samplers are separate bindings (as in Vulkan / D3D / WebGPU).
// OpenGL only has combined texture units, so the GL backend pairs each
// SampledTexture with a Sampler when a bind group is applied.
//
// Dynamic offsets are a per-entry flag on the bind group layout, not a
// separate binding type.
//
// WebGPU: Binding
// Vulkan: VkDescriptorType
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class BindType : uint8_t {
    UniformBuffer         = 0,
    StorageBuffer         = 1,
    ReadOnlyStorageBuffer = 2,
    SampledTexture        = 3,
    Sampler               = 4,
    StorageTexture        = 5,
};

} // namespace Nezumi::RHI
