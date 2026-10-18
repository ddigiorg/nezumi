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
// GLRenderPipeline
// -----------------------------------------------------------------------------

struct GLRenderPipeline {
    GLuint program  = 0;
    GLuint vao      = 0;
    GLenum topology = GL_TRIANGLES;

    // Vertex buffer strides by slot. GL takes the stride when a buffer is
    // bound, not as part of the layout, so it's kept here for draw time.
    std::array<GLsizei, MAX_VERTEX_BUFFERS> strides = {};
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
