// =============================================================================
// GLRenderPipeline.hpp
//
// The OpenGL backend's storage for an RHI graphics pipeline: a linked program,
// a vertex array object describing the vertex layout, and the fixed-function
// state, already converted to GL enums so binding a pipeline does no work
// beyond the GL calls themselves.
// =============================================================================

#pragma once
#include "Nezumi/Object/SlotMap.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLLimits.hpp"
#include "Nezumi/RHI/BindGroup/BindGroupLayoutHandle.hpp"
#include "Nezumi/RHI/Common/Format.hpp"
#include "Nezumi/RHI/Pipeline/RenderPipelineHandle.hpp"
#include <array>
#include <cstdint>
#include <glad/gl.h>
#include <string>
#include <vector>

namespace Nezumi::RHI::GL {

// -----------------------------------------------------------------------------
// Fixed-function state, grouped the way GLStateCache applies it. Each group is
// compared as a whole and re-applied only when it differs.
// -----------------------------------------------------------------------------

struct GLRasterState {
    bool   cull_enabled = false;
    GLenum cull_face    = GL_BACK;
    GLenum front_face   = GL_CCW;

    bool operator==(const GLRasterState&) const noexcept = default;
};

struct GLStencilFace {
    GLenum func       = GL_ALWAYS;
    GLenum fail       = GL_KEEP;
    GLenum depth_fail = GL_KEEP;
    GLenum pass       = GL_KEEP;

    bool operator==(const GLStencilFace&) const noexcept = default;
};

struct GLDepthStencilState {
    bool          depth_test   = false;
    GLboolean     depth_write  = GL_FALSE;
    GLenum        depth_func   = GL_ALWAYS;
    bool          stencil_test = false;
    GLStencilFace front        = {};
    GLStencilFace back         = {};
    GLuint        read_mask    = 0xFF;
    GLuint        write_mask   = 0xFF;

    bool operator==(const GLDepthStencilState&) const noexcept = default;
};

struct GLBlendTarget {
    bool   enabled   = false;
    GLenum src_color = GL_ONE;
    GLenum dst_color = GL_ZERO;
    GLenum op_color  = GL_FUNC_ADD;
    GLenum src_alpha = GL_ONE;
    GLenum dst_alpha = GL_ZERO;
    GLenum op_alpha  = GL_FUNC_ADD;

    bool operator==(const GLBlendTarget&) const noexcept = default;
};

struct GLColorMask {
    GLboolean r = GL_TRUE;
    GLboolean g = GL_TRUE;
    GLboolean b = GL_TRUE;
    GLboolean a = GL_TRUE;

    bool operator==(const GLColorMask&) const noexcept = default;
};

// Everything GLStateCache applies when a pipeline is bound.
struct GLRenderPipelineState {
    GLRasterState                                     raster             = {};
    GLDepthStencilState                               depth_stencil      = {};
    std::array<GLBlendTarget, MAX_COLOR_ATTACHMENTS>  blend              = {};
    std::array<GLColorMask,   MAX_COLOR_ATTACHMENTS>  color_mask         = {};
    std::uint32_t                                     color_target_count = 0;
};

// -----------------------------------------------------------------------------
// Vertex input
//
// GL 3.3 has no separate vertex format and buffer binding (that's 4.3's
// glVertexAttribFormat / glBindVertexBuffer): each attribute is pointed at
// its buffer with glVertexAttribPointer, which takes the buffer, offset, and
// stride all at once. So the layout is kept here and applied at draw time,
// once the buffers are known.
// -----------------------------------------------------------------------------

struct GLVertexAttribute {
    GLuint    location   = 0;
    GLint     size       = 0;        // components
    GLenum    type       = GL_FLOAT;
    GLboolean normalized = GL_FALSE;
    bool      integer    = false;    // glVertexAttribIPointer
    GLuint    offset     = 0;        // within one vertex / instance
};

struct GLVertexSlot {
    GLsizei       stride          = 0;
    bool          per_instance    = false;
    std::uint32_t first_attribute = 0;   // into GLRenderPipeline::attributes
    std::uint32_t attribute_count = 0;
};

// -----------------------------------------------------------------------------
// GLRenderPipeline
// -----------------------------------------------------------------------------

struct GLRenderPipeline {
    GLuint program  = 0;
    GLuint vao      = 0;
    GLenum topology = GL_TRIANGLES;

    // Vertex layout by slot, applied at draw time (see above). The VAO holds
    // only which attributes are enabled and their divisors.
    std::array<GLVertexSlot, MAX_VERTEX_BUFFERS> vertex_slots = {};
    std::vector<GLVertexAttribute>               attributes;
    std::uint32_t vertex_buffer_count = 0;

    GLRenderPipelineState state = {};

    // Attachment formats this pipeline renders to; must match the render
    // pass it's used in (as Vulkan and D3D12 require).
    std::array<Format, MAX_COLOR_ATTACHMENTS> color_formats = {};
    Format depth_format  = Format::Undefined;

    // Used by bind groups (part 3).
    std::array<BindGroupLayoutHandle, MAX_BIND_GROUPS> bind_group_layouts = {};
    std::uint32_t bind_group_count   = 0;

    // See GLTexture::pending_destroy.
    bool pending_destroy = false;

    std::string name;
};

using GLPipelinePool = SlotMap<GLRenderPipeline, RenderPipelineTag>;

} // namespace Nezumi::RHI::GL
