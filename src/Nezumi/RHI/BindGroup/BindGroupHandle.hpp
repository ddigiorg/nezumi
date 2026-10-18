// =============================================================================
// BindGroupHandle.hpp
//
// A handle to a collection of GPU resources (e.g. buffers and textures) that
// are bound together and made accessible to shaders.
//
// WebGPU: GPUBindGroup
// Vulkan: VkDescriptorSet
// =============================================================================

#pragma once
#include "Nezumi/Object/Handle.hpp"

namespace Nezumi::RHI {

struct BindGroupTag;
using  BindGroupHandle = Handle<BindGroupTag>;

} // namespace Nezumi::RHI
