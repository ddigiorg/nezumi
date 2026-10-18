// =============================================================================
// SamplerHandler.hpp
//
// A handler to an object that defines how shaders read and filter texels from
// an image.
//
// WebGPU: GPUSampler
// Vulkan: VkSampler
// =============================================================================

#pragma once
#include "Nezumi/Object/Handle.hpp"

namespace Nezumi::RHI {

struct SamplerTag;
using  SamplerHandle = Handle<SamplerTag>;

} // namespace Nezumi::RHI
