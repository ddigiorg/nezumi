// =============================================================================
// BindGroupLayoutHandle.hpp
//
// A handle to a blueprint that defines the structure, types, and shader stages
// of resources (e.g. buffers and textures) before they are bound.  This allows
// the pipeline to know what to expect for each binding.
//
// WebGPU: GPUBindGroupLayout
// Vulkan: VkDescriptorSetLayout
// =============================================================================

#pragma once
#include "Nezumi/Object/Handle.hpp"

namespace Nezumi::RHI {

struct BindGroupLayoutTag;
using  BindGroupLayoutHandle = Handle<BindGroupLayoutTag>;

} // namespace Nezumi::RHI
