// =============================================================================
// RenderCore.cpp
// =============================================================================

#include "Nezumi/Render/Core/RenderCore.hpp"
#include <algorithm> // find, max, remove
#include <cassert>   // assert
#include <cmath>     // lround
#include <stdexcept> // runtime_error

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

RenderCore::~RenderCore() {
    for (TargetSlot& slot : targets_) device_.remove(slot.texture);
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

    // Targets follow the backbuffer, now that its size is final for this frame
    resize_targets_();

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
        const RHI::TextureHandle target = target_texture(pass.target);
        if (!target) continue; // a zero-sized backbuffer (minimized)

        const RHI::ColorAttachment color[] = {{
            .texture     = target,
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

        PassContext ctx = {
            .name   = pass.name,
            .target = pass.target,
            .format = target_format(pass.target),
            .extent = target_extent(pass.target),
        };

        for (const PassFeature& entry : pass.features) {
            assert(entry.feature &&
                   std::find(features_.begin(), features_.end(), entry.feature) != features_.end() &&
                   "RenderCore: pass lists a feature that isn't registered");
            ctx.views = entry.views;
            ctx.step  = entry.step;
            entry.feature->record(*this, encoder, ctx);
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
// Targets
// -----------------------------------------------------------------------------

TargetID RenderCore::create_target(const RenderTargetConfig& config) {
    assert(!in_frame_ && "RenderCore: creating a target during a frame");
    if (targets_.size() >= 0xFE) {
        throw std::runtime_error("RenderCore: out of render targets (254)");
    }
    if (!(config.scale > 0.0f)) {
        throw std::runtime_error("RenderCore: target '" + config.name + "' needs a scale > 0");
    }

    TargetSlot& slot = targets_.emplace_back(TargetSlot{ .config = config });
    create_target_texture_(slot, scaled_extent_(config.scale));
    return static_cast<TargetID>(targets_.size());
}

RHI::TextureHandle RenderCore::target_texture(TargetID target) const {
    if (target == Target::Backbuffer) {
        const RHI::Extent2D size = device_.backbuffer_extent();
        return (size.width > 0 && size.height > 0) ? device_.backbuffer() : RHI::TextureHandle{};
    }
    assert(target <= targets_.size() && "RenderCore: unknown target");
    return targets_[target - 1].texture;
}

RHI::Format RenderCore::target_format(TargetID target) const {
    if (target == Target::Backbuffer) return device_.backbuffer_format();
    assert(target <= targets_.size() && "RenderCore: unknown target");
    return targets_[target - 1].config.format;
}

RHI::Extent2D RenderCore::target_extent(TargetID target) const {
    if (target == Target::Backbuffer) return device_.backbuffer_extent();
    assert(target <= targets_.size() && "RenderCore: unknown target");
    return targets_[target - 1].extent;
}

const RenderTargetConfig& RenderCore::target_config(TargetID target) const {
    assert(target != Target::Backbuffer && target <= targets_.size() && "RenderCore: unknown target");
    return targets_[target - 1].config;
}

RHI::Extent2D RenderCore::scaled_extent_(float scale) const {
    const RHI::Extent2D size = device_.backbuffer_extent();
    const auto scaled = [&](std::uint32_t n) {
        return static_cast<std::uint32_t>(std::max(1L, std::lround(float(n) * scale)));
    };
    return { scaled(std::max<std::uint32_t>(size.width,  1)),
             scaled(std::max<std::uint32_t>(size.height, 1)) };
}

void RenderCore::create_target_texture_(TargetSlot& slot, RHI::Extent2D extent) {
    device_.remove(slot.texture); // deferred: the GPU may still read the old one

    slot.extent  = extent;
    slot.texture = device_.create(
        RHI::TextureConfig{
            .name   = slot.config.name,
            .format = slot.config.format,
            .width  = extent.width,
            .height = extent.height,
            .usage  = RHI::TextureUsageFlags::ColorTarget | RHI::TextureUsageFlags::Sampled,
        }
    );
    if (!slot.texture) {
        throw std::runtime_error(
            "RenderCore: target '" + slot.config.name + "' failed (see log)");
    }
}

void RenderCore::resize_targets_() {
    const RHI::Extent2D size = device_.backbuffer_extent();
    if (size.width == 0 || size.height == 0) return; // minimized: keep the old ones

    for (TargetSlot& slot : targets_) {
        const RHI::Extent2D want = scaled_extent_(slot.config.scale);
        if (want.width != slot.extent.width || want.height != slot.extent.height) {
            create_target_texture_(slot, want);
        }
    }
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
