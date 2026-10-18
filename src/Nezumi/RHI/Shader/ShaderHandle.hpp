// =============================================================================
// ShaderHandle.hpp
//
// A handler to a specialized computer program that runs on a graphics
// processing unit (GPU).
//
// WebGPU: GPUShaderModule
// Vulkan: VkShaderModule
// =============================================================================

#pragma once
#include "Nezumi/Object/Handle.hpp"

namespace Nezumi::RHI {

struct ShaderTag;
using  ShaderHandle = Handle<ShaderTag>;

} // namespace Nezumi::RHI
