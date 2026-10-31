// =============================================================================
// RenderCore.hpp
//
// The shared half of rendering: GPU services every feature renderer uses, and
// ownership of the frame. Feature renderers (Renderer2D today, a mesh renderer
// later) register with it and record into the passes of its FrameRecipe.
//
//   RenderCore core(device);
//   Renderer2D r2d(core);                    // registers itself
//   core.set_recipe({ .passes = {{ .name = "main", .features = {{ &r2d }} }} });
//
//   core.begin_frame();
//   r2d.draw_sprite(...);                    // any feature, any order
//   core.end_frame();                        // prepare, upload, run passes
//
// Owns:
//   - MaterialTable  bind group 1: texture + sampler per MaterialID
//   - ShaderLibrary  shader cache by path
//   - StreamBuffer   per-frame instance data, shared by all features
//   - ViewBindings   bind group 0: one uniform slot per ViewID
//   - FrameRecipe    the passes, in order
//
// Views: each frame, features publish the views they draw through with
// set_view(); RenderCore uploads exactly those. A view nobody publishes keeps
// last frame's matrices, so publish every view you record.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/Frame/FrameRecipe.hpp"
#include "Nezumi/Render/Core/Frame/IRenderFeature.hpp"
#include "Nezumi/Render/Core/Frame/StreamBuffer.hpp"
#include "Nezumi/Render/Core/Material/MaterialTable.hpp"
#include "Nezumi/Render/Core/Shader/ShaderLibrary.hpp"
#include "Nezumi/Render/Core/View/ViewBindings.hpp"
#include "Nezumi/Render/Core/View/ViewData.hpp"
#include "Nezumi/Render/Core/View/ViewID.hpp"
#include "Nezumi/Render/Core/RenderContext.hpp"
#include "Nezumi/Render/Core/RenderCoreConfig.hpp"
#include "Nezumi/RHI/RHI.hpp"
#include <array>  // array
#include <vector> // vector

namespace Nezumi::Render {

class RenderCore {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    explicit RenderCore(RHI::Device& device, const RenderCoreConfig& config = {});
    ~RenderCore() = default;

    // Non-copyable and non-movable
    RenderCore(const RenderCore&)            = delete;
    RenderCore& operator=(const RenderCore&) = delete;
    RenderCore(RenderCore&&)                 = delete;
    RenderCore& operator=(RenderCore&&)      = delete;

    // -------------------------------------------------------------------------
    // Frame Control
    //
    // All GPU work happens during end_frame().
    // -------------------------------------------------------------------------

    void begin_frame();
    void end_frame();

    bool in_frame() const { return in_frame_; }

    // -------------------------------------------------------------------------
    // Features
    //
    // Registered features get every frame hook (see IRenderFeature), whether
    // or not a pass lists them. Neither is owned; remove a feature before
    // destroying it.
    // -------------------------------------------------------------------------

    void add_feature   (IRenderFeature& feature);
    void remove_feature(IRenderFeature& feature);

    // -------------------------------------------------------------------------
    // Recipe
    //
    // Every feature a pass lists must be registered.
    // -------------------------------------------------------------------------

    void               set_recipe(FrameRecipe recipe);
    const FrameRecipe& get_recipe() const { return recipe_; }

    // -------------------------------------------------------------------------
    // Views
    //
    // Publish a view's matrices for this frame. Call from a feature's upload()
    // hook (or anywhere between begin_frame and the upload step).
    // -------------------------------------------------------------------------

    void set_view(ViewID view, const ViewData& data);

    // -------------------------------------------------------------------------
    // Display Scale
    //
    // The screen's HiDPI factor (Window::get_display_scale, set by the Engine
    // before each frame). Features scale their screen-space content by it.
    // -------------------------------------------------------------------------

    void  set_display_scale(float scale) { display_scale_ = scale > 0.0f ? scale : 1.0f; }
    float get_display_scale() const      { return display_scale_; }

    // -------------------------------------------------------------------------
    // Services
    // -------------------------------------------------------------------------

    RHI::Device&            device()          { return device_;    }
    const RenderCoreConfig& config()    const { return config_;    }
    RenderContext&          context()         { return context_;   }
    MaterialTable&          materials()       { return materials_; }
    ShaderLibrary&          shaders()         { return shaders_;   }
    StreamBuffer&           stream()          { return stream_;    }
    ViewBindings&           views()           { return views_;     }

private:

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RHI::Device&     device_;
    RenderCoreConfig config_;
    RenderContext    context_;

    MaterialTable    materials_;
    ShaderLibrary    shaders_;
    StreamBuffer     stream_;
    ViewBindings     views_;

    FrameRecipe                  recipe_;
    std::vector<IRenderFeature*> features_;

    std::array<ViewData, VIEW_CAPACITY> view_data_ = {};
    ViewMask                            view_dirty_ = 0;

    bool  in_frame_      = false;
    float display_scale_ = 1.0f;
};

} // namespace Nezumi::Render
