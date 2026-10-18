// =============================================================================
// TextureHandle.hpp
//
// A handle to an image or volumetric data.
//
// WebGPU: GPUTexture
// Vulkan: VkImage
// =============================================================================

#pragma once
#include "Nezumi/Object/Handle.hpp"

namespace Nezumi::RHI {

struct TextureTag;
using  TextureHandle = Handle<TextureTag>;

} // namespace Nezumi::RHI
