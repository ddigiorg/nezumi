// =============================================================================
// GLEnumConvert.hpp
//
// RHI enum -> OpenGL enum conversions. Each is an exhaustive switch, so with
// -Werror=switch a new RHI enum value is a compile error here until handled.
// =============================================================================
#pragma once
#include "Nezumi/RHI/Buffer/IndexFormat.hpp"
#include "Nezumi/RHI/Common/Format.hpp"
#include "Nezumi/RHI/Pipeline/BlendFactor.hpp"
#include "Nezumi/RHI/Pipeline/BlendOp.hpp"
#include "Nezumi/RHI/Pipeline/CompareOp.hpp"
#include "Nezumi/RHI/Pipeline/CullMode.hpp"
#include "Nezumi/RHI/Pipeline/FrontFace.hpp"
#include "Nezumi/RHI/Pipeline/PrimitiveTopology.hpp"
#include "Nezumi/RHI/Pipeline/StencilOp.hpp"
#include "Nezumi/RHI/Pipeline/VertexFormat.hpp"
#include "Nezumi/RHI/Sampler/SamplerAddressMode.hpp"
#include "Nezumi/RHI/Sampler/SamplerFilterMode.hpp"
#include "Nezumi/RHI/Shader/ShaderStageFlags.hpp"
#include "Nezumi/RHI/Texture/TextureType.hpp"
#include <glad/gl.h>
#include <cstdint>

namespace Nezumi::RHI::GL {

// -----------------------------------------------------------------------------
// Formats
// -----------------------------------------------------------------------------

// Sized internal format for glTextureStorage*. GL_NONE for Format::Undefined.
GLenum to_gl_internal_format(Format format) noexcept;

// How pixel data handed to glTextureSubImage* is laid out in memory.
struct GLUploadFormat {
    GLenum format = GL_NONE;   // channels: GL_RED, GL_RGBA, GL_BGRA, ...
    GLenum type   = GL_NONE;   // component type: GL_UNSIGNED_BYTE, GL_FLOAT, ...
};

GLUploadFormat to_gl_upload_format(Format format) noexcept;

// -----------------------------------------------------------------------------
// Textures and samplers
// -----------------------------------------------------------------------------

GLenum to_gl_texture_target(TextureType type) noexcept;

// GL folds the mip filter into the minification filter.
GLenum to_gl_min_filter(SamplerFilterMode min, SamplerFilterMode mip) noexcept;
GLenum to_gl_mag_filter(SamplerFilterMode mag) noexcept;
GLenum to_gl_address_mode(SamplerAddressMode mode) noexcept;

// -----------------------------------------------------------------------------
// Shaders
// -----------------------------------------------------------------------------

// GL_VERTEX_SHADER etc. GL_NONE unless exactly one stage bit is set.
GLenum to_gl_shader_stage(ShaderStageFlags stage) noexcept;

// -----------------------------------------------------------------------------
// Vertex input
// -----------------------------------------------------------------------------

// How one vertex attribute is described to glVertexArrayAttrib*Format.
struct GLVertexFormat {
    GLint     size       = 0;           // component count
    GLenum    type       = GL_NONE;     // component type
    GLboolean normalized = GL_FALSE;    // integer -> [0, 1] float
    bool      integer    = false;       // true: glVertexArrayAttribIFormat (ivec/uvec in GLSL)
};

GLVertexFormat to_gl_vertex_format(VertexFormat format) noexcept;

GLenum        to_gl_index_type(IndexFormat format) noexcept;
std::uint32_t index_size(IndexFormat format) noexcept;   // bytes

// -----------------------------------------------------------------------------
// Fixed-function state
// -----------------------------------------------------------------------------

GLenum to_gl_primitive_topology(PrimitiveTopology topology) noexcept;
GLenum to_gl_cull_face(CullMode mode) noexcept;   // CullMode::None -> GL_BACK (culling disabled separately)
GLenum to_gl_front_face(FrontFace face) noexcept;
GLenum to_gl_compare_op(CompareOp op) noexcept;
GLenum to_gl_stencil_op(StencilOp op) noexcept;
GLenum to_gl_blend_factor(BlendFactor factor) noexcept;
GLenum to_gl_blend_op(BlendOp op) noexcept;

} // namespace Nezumi::RHI::GL
