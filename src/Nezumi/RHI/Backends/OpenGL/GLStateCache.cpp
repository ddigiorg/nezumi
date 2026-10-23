// =============================================================================
// GLStateCache.cpp
// =============================================================================

#include "Nezumi/RHI/Backends/OpenGL/GLStateCache.hpp"

namespace Nezumi::RHI::GL {

namespace {

void set_enabled(GLenum capability, bool enabled) {
    if (enabled) glEnable(capability); else glDisable(capability);
}

} // namespace

void GLStateCache::apply(const GLRenderPipelineState& state) {
    // Rasterizer
    if (!raster_valid_ || raster_ != state.raster) {
        set_enabled(GL_CULL_FACE, state.raster.cull_enabled);
        glCullFace(state.raster.cull_face);
        glFrontFace(state.raster.front_face);
        raster_       = state.raster;
        raster_valid_ = true;
    }

    // Depth and stencil
    if (!depth_stencil_valid_ || depth_stencil_ != state.depth_stencil) {
        const GLDepthStencilState& ds = state.depth_stencil;

        // As in Vulkan and D3D, depth writes only happen while the depth test
        // is enabled.
        set_enabled(GL_DEPTH_TEST, ds.depth_test);
        glDepthMask(ds.depth_write);
        glDepthFunc(ds.depth_func);

        set_enabled(GL_STENCIL_TEST, ds.stencil_test);
        glStencilOpSeparate(GL_FRONT, ds.front.fail, ds.front.depth_fail, ds.front.pass);
        glStencilOpSeparate(GL_BACK,  ds.back.fail,  ds.back.depth_fail,  ds.back.pass);
        glStencilMask(ds.write_mask);

        depth_stencil_       = ds;
        depth_stencil_valid_ = true;
        apply_stencil_funcs_();
    }

    // Per color target. Targets past color_target_count don't exist in the
    // pass's framebuffer, so their state doesn't matter.
    //
    // GL 3.3 enables blending per target but has one blend function for all
    // of them (glBlendFuncSeparatei is 4.0). GLDevice rejects pipelines whose
    // enabled targets disagree, so the first enabled target's function is
    // the pipeline's.
    for (std::uint32_t i = 0; i < state.color_target_count; ++i) {
        const GLBlendTarget& blend = state.blend[i];
        if (!blend_valid_[i] || blend_[i].enabled != blend.enabled) {
            if (blend.enabled) glEnablei(GL_BLEND, i); else glDisablei(GL_BLEND, i);
            blend_[i].enabled = blend.enabled;
            blend_valid_[i]   = true;
        }

        if (blend.enabled && (!blend_func_valid_ || blend_func_ != blend)) {
            glBlendFuncSeparate(blend.src_color, blend.dst_color, blend.src_alpha, blend.dst_alpha);
            glBlendEquationSeparate(blend.op_color, blend.op_alpha);
            blend_func_       = blend;
            blend_func_valid_ = true;
        }

        const GLColorMask& mask = state.color_mask[i];
        if (!mask_valid_[i] || color_mask_[i] != mask) {
            glColorMaski(i, mask.r, mask.g, mask.b, mask.a);
            color_mask_[i] = mask;
            mask_valid_[i] = true;
        }
    }
}

void GLStateCache::set_stencil_reference(GLint reference) {
    if (reference == stencil_reference_) return;
    stencil_reference_ = reference;
    if (depth_stencil_valid_) apply_stencil_funcs_();
}

void GLStateCache::write_masks_changed() {
    // Depth and stencil write masks live in the depth-stencil group.
    depth_stencil_valid_ = false;
    mask_valid_.fill(false);
}

void GLStateCache::reset() {
    raster_valid_        = false;
    depth_stencil_valid_ = false;
    blend_valid_.fill(false);
    blend_func_valid_ = false;
    mask_valid_.fill(false);
}

void GLStateCache::apply_stencil_funcs_() const {
    glStencilFuncSeparate(
        GL_FRONT,
        depth_stencil_.front.func,
        stencil_reference_,
        depth_stencil_.read_mask
    );

    glStencilFuncSeparate(
        GL_BACK,
        depth_stencil_.back.func,
        stencil_reference_,
        depth_stencil_.read_mask
    );
}

} // namespace Nezumi::RHI::GL
