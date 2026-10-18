// =============================================================================
// GLBuffer.hpp
//
// The OpenGL backend's storage for an RHI buffer. Plain data with no
// destructor: GLDevice creates and deletes the GL object explicitly.
// =============================================================================

#pragma once
#include "Nezumi/Object/SlotMap.hpp"
#include "Nezumi/RHI/Buffer/BufferHandle.hpp"
#include "Nezumi/RHI/Buffer/BufferUsageFlags.hpp"
#include <cstdint>   // uint64_t
#include <glad/gl.h>
#include <string>    // string

namespace Nezumi::RHI::GL {

struct GLBuffer {
    GLuint           id    = 0;
    std::uint64_t    size  = 0;
    BufferUsageFlags usage = BufferUsageFlags::None;

    // See GLTexture::pending_destroy.
    bool pending_destroy = false;

    std::string name;
};

using GLBufferPool = SlotMap<GLBuffer, BufferTag>;

} // namespace Nezumi::RHI::GL
