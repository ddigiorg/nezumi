// =============================================================================
// GLShaderBindings.hpp
//
// GLSL 3.30 can't declare binding points in the shader: the binding layout
// qualifier arrived in GLSL 4.20. Nezumi's shaders still declare them the
// Vulkan way (see the binding rule in GLLimits.hpp):
//
//     layout(std140, binding = 0) uniform Frame { mat4 view_proj; };
//     layout(binding = 16) uniform sampler2D u_atlas;
//
// so the backend removes each `binding = N` before compiling, remembers which
// uniform it belonged to, and assigns the binding after the program links.
// =============================================================================

#pragma once
#include <cstdint>
#include <glad/gl.h>
#include <span>
#include <string>
#include <vector>

namespace Nezumi::RHI::GL {

struct GLUniformBinding {
    std::string   name;            // block name, or the sampler uniform's name
    std::uint32_t binding = 0;     // GL binding point (group * 16 + binding)
    bool          block   = false; // uniform block (true) or sampler (false)
};

// Removes `binding = N` from every `layout(...) uniform` declaration in
// `source` (an empty `layout()` is removed entirely) and returns what was
// removed. Comments are ignored. Line numbers are preserved, so compiler
// errors still point at the right line.
std::vector<GLUniformBinding> strip_binding_qualifiers(std::string& source);

// Points each uniform block and sampler of a linked program at its binding.
// Uniforms the linker optimized away are skipped. Changes the current
// program and restores it.
void apply_bindings(GLuint program, std::span<const GLUniformBinding> bindings);

} // namespace Nezumi::RHI::GL
