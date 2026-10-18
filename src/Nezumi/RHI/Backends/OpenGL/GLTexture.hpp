// =============================================================================
// GLTexture.hpp
//
// The OpenGL backend's storage for an RHI texture. Plain data with no
// destructor: GLDevice creates and deletes the GL object explicitly.
// =============================================================================

#pragma once
#include "Nezumi/Object/SlotMap.hpp"
#include "Nezumi/RHI/Common/Format.hpp"
#include "Nezumi/RHI/Texture/TextureHandle.hpp"
#include "Nezumi/RHI/Texture/TextureType.hpp"
#include "Nezumi/RHI/Texture/TextureUsageFlags.hpp"
#include <cstdint>
#include <glad/gl.h>
#include <string>

namespace Nezumi::RHI::GL {

struct GLTexture {
    GLuint        id              = 0;
    GLenum        target          = 0; // GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, ...
    TextureType   type            = TextureType::Texture2D;
    Format        format          = Format::Undefined;
    std::uint32_t width           = 0;
    std::uint32_t height          = 0;
    std::uint32_t depth_or_layers = 1;
    std::uint32_t mip_levels      = 1;
    TextureUsageFlags usage       = TextureUsageFlags::None;

    // Destroyed during a frame: still resolvable by that frame's replay, but
    // rejected by everything else. Deleted at the end of the frame.
    bool pending_destroy = false;

    std::string name; // owned copy of the config's name
};

using GLTexturePool = SlotMap<GLTexture, TextureTag>;

} // namespace Nezumi::RHI::GL
