// =============================================================================
// GLLimits.hpp
//
// Fixed limits the OpenGL backend sizes its arrays by, and the rule for
// mapping bind groups onto OpenGL binding points.
// =============================================================================

#pragma once
#include <cstdint> // uint32_t

namespace Nezumi::RHI::GL {

// 3.3 core: VAOs, sampler objects, uniform buffers, instancing with
// divisors, and explicit attribute locations in GLSL. Newer features
// (glClipControl, KHR_debug, anisotropy, framebuffer invalidation) are
// used through extensions when the driver has them.
inline constexpr int REQUIRED_MAJOR = 3;
inline constexpr int REQUIRED_MINOR = 3;

inline constexpr std::uint32_t MAX_COLOR_ATTACHMENTS = 8;  // GL_MAX_COLOR_ATTACHMENTS >= 8
inline constexpr std::uint32_t MAX_VERTEX_BUFFERS    = 16; // RHI slots; GL 3.3 binds buffers per attribute
inline constexpr std::uint32_t MAX_BIND_GROUPS       = 4;  // RHI convention (DeviceInfo::max_bind_groups)
inline constexpr std::uint32_t BINDINGS_PER_GROUP    = 16; // bindings 0..15 in each group

// =============================================================================
// Binding rule for GLSL
//
// OpenGL has no bind groups: each resource type has one flat range of
// binding points. Each (group, binding) pair maps onto
//
//     GL binding = group * 16 + binding
//
// GLSL 3.30 can't declare binding points (layout(binding = N) is 4.20), so
// shaders declare plain uniforms and each BindGroupLayoutEntry carries the
// GLSL name. When a pipeline is created, GLDevice looks each name up and
// assigns the binding (glUniformBlockBinding for blocks, glUniform1i for
// samplers):
//
//     layout(std140) uniform Frame { mat4 view_proj; };  // entry { .binding = 0, .name = "Frame" }, group 0 -> 0
//     uniform sampler2D u_texture;                       // entry { .binding = 0, .name = "u_texture" }, group 1 -> 16
//
// Samplers: GLSL's sampler2D combines a texture and a sampler, while the RHI
// keeps them separate. A group's Sampler entry applies to every
// SampledTexture in the same group, so the sampler binding itself has no
// GLSL declaration. A group containing textures may have at most one sampler.
//
// Storage buffers and storage textures need GL 4.3 / 4.2 and are rejected
// by the 3.3 backend.
// =============================================================================

constexpr std::uint32_t gl_binding(std::uint32_t group, std::uint32_t binding) noexcept {
    return group * BINDINGS_PER_GROUP + binding;
}

} // namespace Nezumi::RHI::GL
