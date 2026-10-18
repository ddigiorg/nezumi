// =============================================================================
// GLShader.hpp
//
// The OpenGL backend's storage for an RHI shader: one compiled stage. Stages
// are linked into a GL program when a pipeline is created.
// =============================================================================

#pragma once
#include "Nezumi/Object/SlotMap.hpp"
#include "Nezumi/RHI/Shader/ShaderHandle.hpp"
#include "Nezumi/RHI/Shader/ShaderStageFlags.hpp"
#include <glad/gl.h>
#include <string>

namespace Nezumi::RHI::GL {

struct GLShader {
    GLuint           id    = 0;
    ShaderStageFlags stage = ShaderStageFlags::None;

    // See GLTexture::pending_destroy.
    bool pending_destroy = false;

    std::string name;
};

using GLShaderPool = SlotMap<GLShader, ShaderTag>;

} // namespace Nezumi::RHI::GL
