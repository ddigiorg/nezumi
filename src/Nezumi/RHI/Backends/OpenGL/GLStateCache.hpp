// =============================================================================
// GLStateCache.hpp
//
// OpenGL is one big global state machine, and most GL state calls cost the
// driver something even when the value doesn't change. The cache remembers
// what's currently set and only issues calls for state that differs, so
// switching between pipelines that share, say, the same blend mode costs
// nothing for blending.
//
// Anything that changes this state without going through the cache must
// tell it (see write_masks_changed), or the cache will skip a call it
// thinks is redundant and render with the wrong state.
// =============================================================================

#pragma once
#include "Nezumi/RHI/Backends/OpenGL/GLLimits.hpp"
#include "Nezumi/RHI/Backends/OpenGL/GLRenderPipeline.hpp"
#include <array>
#include <glad/gl.h>

namespace Nezumi::RHI::GL {

class GLStateCache {
public:
    // Apply a pipeline's fixed-function state, skipping unchanged groups.
    void apply(const GLRenderPipelineState& state);

    // Dynamic stencil reference (combined with the pipeline's stencil funcs).
    void set_stencil_reference(GLint reference);

    // Render pass clears open the color, depth, and stencil write masks
    // behind the cache's back; call after clearing.
    void write_masks_changed();

    // Forget everything; the next apply() sets all state.
    void reset();

private:
    void apply_stencil_funcs_() const;

    GLRasterState       raster_        = {};
    GLDepthStencilState depth_stencil_ = {};
    bool                raster_valid_        = false;
    bool                depth_stencil_valid_ = false;

    std::array<GLBlendTarget, MAX_COLOR_ATTACHMENTS> blend_       = {};
    std::array<GLColorMask,   MAX_COLOR_ATTACHMENTS> color_mask_  = {};
    std::array<bool,          MAX_COLOR_ATTACHMENTS> blend_valid_ = {};
    std::array<bool,          MAX_COLOR_ATTACHMENTS> mask_valid_  = {};

    GLint stencil_reference_ = 0;
};

} // namespace Nezumi::RHI::GL
