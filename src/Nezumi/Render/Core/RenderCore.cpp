// =============================================================================
// RenderCore.cpp
// =============================================================================

#include "Nezumi/Render/Core/RenderCore.hpp"
#include <algorithm> // find, remove
#include <cassert>   // assert

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

RenderCore::RenderCore(RHI::Device& device, const RenderCoreConfig& config)
    : device_   (device)
    , config_   (config)
    , materials_(device)
    , shaders_  (device)
    , stream_   (device, config.stream_chunk_size)
    , views_    (device)
{
    context_ = {
        .device    = &device_,
        .views     = &views_,
        .materials = &materials_,
        .shaders   = &shaders_,
        .stream    = &stream_,
    };
}

// -----------------------------------------------------------------------------
// Frame Control
// -----------------------------------------------------------------------------

void RenderCore::begin_frame() {
    assert(!in_frame_ && "RenderCore: begin_frame called twice");
    in_frame_   = true;
    view_dirty_ = 0;
    stream_.reset();

    for (IRenderFeature* feature : features_) {
        feature->begin_frame(*this);
    }
}

void RenderCore::end_frame() {
    assert(in_frame_ && "RenderCore: end_frame without begin_frame");
    in_frame_ = false;

    // -------------------------------------------------------------------------
    // Step 1
    //
    // CPU: each feature sorts, batches and fills the stream mirrors. Done
    // before device_.begin_frame(), which may block on an older frame, so this
    // work overlaps with the GPU.
    // -------------------------------------------------------------------------

    for (IRenderFeature* feature : features_) {
        feature->prepare(*this);
    }

    // -------------------------------------------------------------------------
    // Step 2
    //
    // Upload data from CPU to the device. Inside the device frame, writes are
    // staged and all land before any of this frame's commands run.
    // -------------------------------------------------------------------------

    RHI::CommandEncoder& encoder = device_.begin_frame();

    for (IRenderFeature* feature : features_) {
        feature->upload(*this);
    }

    // Each published view writes its own slot (one write per byte range).
    for (ViewID vid = 0; vid < VIEW_CAPACITY; vid++) {
        if (view_dirty_ & view_bit(vid)) views_.update(vid, view_data_[vid]);
    }

    stream_.flush();

    // -------------------------------------------------------------------------
    // Step 3
    //
    // Record the recipe's passes.
    // -------------------------------------------------------------------------

    for (const PassConfig& pass : recipe_.passes) {
        const RHI::ColorAttachment color[] = {{
            .texture     = device_.backbuffer(),
            .load_op     = pass.clear ? RHI::LoadOp::Clear : RHI::LoadOp::Load,
            .store_op    = RHI::StoreOp::Store,
            .clear_color = pass.clear_color,
        }};

        encoder.begin_render_pass(
            RHI::RenderPassConfig{
                .name              = pass.name,
                .color_attachments = color,
            }
        );

        for (const PassFeature& entry : pass.features) {
            assert(entry.feature &&
                   std::find(features_.begin(), features_.end(), entry.feature) != features_.end() &&
                   "RenderCore: pass lists a feature that isn't registered");
            entry.feature->record(*this, encoder, entry.views);
        }

        encoder.end_render_pass();
    }

    device_.end_frame();

    // -------------------------------------------------------------------------
    // Step 4
    //
    // Let features record frame statistics.
    // -------------------------------------------------------------------------

    for (IRenderFeature* feature : features_) {
        feature->finish(*this);
    }
}

// -----------------------------------------------------------------------------
// Features
// -----------------------------------------------------------------------------

void RenderCore::add_feature(IRenderFeature& feature) {
    assert(!in_frame_ && "RenderCore: adding a feature during a frame");
    if (std::find(features_.begin(), features_.end(), &feature) == features_.end()) {
        features_.push_back(&feature);
    }
}

void RenderCore::remove_feature(IRenderFeature& feature) {
    assert(!in_frame_ && "RenderCore: removing a feature during a frame");
    features_.erase(
        std::remove(features_.begin(), features_.end(), &feature),
        features_.end()
    );

    // Drop it from the recipe too, so no pass points at a dead feature.
    for (PassConfig& pass : recipe_.passes) {
        std::erase_if(pass.features, [&](const PassFeature& e) {
            return e.feature == &feature;
        });
    }
}

// -----------------------------------------------------------------------------
// Recipe
// -----------------------------------------------------------------------------

void RenderCore::set_recipe(FrameRecipe recipe) {
    assert(!in_frame_ && "RenderCore: changing the recipe during a frame");
    recipe_ = std::move(recipe);
}

// -----------------------------------------------------------------------------
// Views
// -----------------------------------------------------------------------------

void RenderCore::set_view(ViewID view, const ViewData& data) {
    assert(view < VIEW_CAPACITY && "RenderCore: view out of range");
    view_data_[view] = data;
    view_dirty_     |= view_bit(view);
}

} // namespace Nezumi::Render
