// =============================================================================
// GLLimits.hpp
//
// Fixed limits the OpenGL backend sizes its arrays by, and the rule for
// mapping bind groups onto OpenGL binding points.
// =============================================================================

#pragma once
#include <cstdint> // uint32_t

namespace Nezumi::RHI::GL {

// 4.5: direct state access (glCreate*, glNamed*) and glClipControl.
inline constexpr int REQUIRED_MAJOR = 4;
inline constexpr int REQUIRED_MINOR = 5;

inline constexpr std::uint32_t MAX_COLOR_ATTACHMENTS = 8;  // GL_MAX_COLOR_ATTACHMENTS >= 8
inline constexpr std::uint32_t MAX_VERTEX_BUFFERS    = 16; // GL_MAX_VERTEX_ATTRIB_BINDINGS >= 16
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
// so GLSL is written with those numbers:
//
//     layout(std140, binding = 0) uniform Frame { mat4 view_proj; };  // group 0, binding 0
//     layout(binding = 16) uniform sampler2D u_atlas;                 // group 1, binding 0
//
// Samplers: GLSL's sampler2D combines a texture and a sampler, while the RHI
// keeps them separate. A group's Sampler entry applies to every
// SampledTexture in the same group, so the sampler binding itself has no
// GLSL declaration. A group containing textures may have at most one sampler.
//
// Storage textures use image units with the same numbering
// (layout(binding = N, rgba8) uniform image2D ...).
// =============================================================================

constexpr std::uint32_t gl_binding(std::uint32_t group, std::uint32_t binding) noexcept {
    return group * BINDINGS_PER_GROUP + binding;
}

} // namespace Nezumi::RHI::GL
