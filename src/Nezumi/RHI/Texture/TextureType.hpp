// =============================================================================
// TextureType.hpp
//
// Specifies the type of a texture.
//
// WebGPU: GPUTextureDimension
// Vulkan: VkImageType
// =============================================================================

#pragma once
#include <cstdint> // uint8_t

namespace Nezumi::RHI {

enum class TextureType : uint8_t {
    Texture1D      = 0, // A single 1D line of texels
    Texture2D      = 1, // A single 2D image of texels
    Texture2DArray = 2, // A stack of multiple independent 2D image layers (good for texture atlases).
    TextureCube    = 3, // Six 2D square images representing the faces of a cube (for skybox or cubemap)
    //TextureCubeArray  // Not implemented
    Texture3D      = 4, // A volumetric grid of data (voxels) across width, height, and a variable depth slice
};

} // namespace Nezumi::RHI
