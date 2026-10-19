// =============================================================================
// GLShader.hpp
//
// The OpenGL backend's storage for an RHI shader: one compiled stage. Stages
// are linked into a GL program when a pipeline is created.
// =============================================================================

#pragma once
#include "Nezumi/Object/SlotMap.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLShaderBindings.hpp"
#include "Nezumi/RHI/Shader/ShaderHandle.hpp"
#include "Nezumi/RHI/Shader/ShaderStageFlags.hpp"
#include <glad/gl.h>
#include <string>
#include <vector>

namespace Nezumi::RHI::GL {

struct GLShader {
    GLuint           id    = 0;
    ShaderStageFlags stage = ShaderStageFlags::None;

    // Binding qualifiers stripped from the source, applied once the stage
    // is linked into a program (see GLShaderBindings.hpp).
    std::vector<GLUniformBinding> bindings;

    // See GLTexture::pending_destroy.
    bool pending_destroy = false;

    std::string name;
};

using GLShaderPool = SlotMap<GLShader, ShaderTag>;

} // namespace Nezumi::RHI::GL
