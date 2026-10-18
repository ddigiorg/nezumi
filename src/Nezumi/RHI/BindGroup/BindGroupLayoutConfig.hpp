// =============================================================================
// BindGroupLayoutConfig.hpp
//
// Config of a BindGroupLayout.
//
// A layout is the slot signature a shader expects for one group:
// "binding 0 is a uniform buffer seen by the vertex stage, binding 1 is a
// sampled texture seen by the fragment stage", and so on.
//
// Suggested convention:
//   group 0 = per frame    (camera, time)
//   group 1 = per material (texture + sampler)
//   group 2 = per draw
//
// WebGPU: GPUBindGroupLayoutDescriptor
// Vulkan: VkDescriptorSetLayoutCreateInfo
// =============================================================================

#pragma once
#include "Nezumi/RHI/BindGroup/BindGroupLayoutEntry.hpp"
#include <span>        // span
#include <string_view> // string_view

namespace Nezumi::RHI {

struct BindGroupLayoutConfig {
    std::string_view                      name    = "";
    std::span<const BindGroupLayoutEntry> entries = {};
};

} // namespace Nezumi::RHI
