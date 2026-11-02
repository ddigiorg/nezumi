// =============================================================================
// PostProcess.hpp
//
// The lighting composite, bloom, and tonemap: fullscreen steps that turn a
// scene target and a light target into the final image. One IRenderFeature
// that records a different step in each of its passes (PassFeature::step);
// it hands out those passes so the recipe can't get them wrong.
//
// A lit 2D frame (see examples/07-lighting):
//
//   const TargetID scene = core.create_target({ .name = "scene" });
//   const TargetID light = core.create_target({ .name = "light", .scale = 0.5f });
//   PostProcess post(core, { .scene = scene, .light = light });
//
//   FrameRecipe recipe;
//   recipe.passes.push_back({ .name = "scene",  .target = scene,
//                             .features = { { &r2d, view_bit(View::World) } } });
//   recipe.passes.push_back({ .name = "lights", .target = light,
//                             .clear_color = { 0, 0, 0, 0 },
//                             .features = { { &r2d, view_bit(View::Lights) } } });
//   recipe.passes.push_back(post.composite_pass());          // lit = scene x light
//   recipe.passes.push_back({ .name = "emissive", .target = post.lit_target(),
//                             .clear = false,
//                             .features = { { &r2d, view_bit(View::Emissive) } } });
//   for (PassConfig& p : post.output_passes()) recipe.passes.push_back(std::move(p));
//   recipe.passes.push_back({ .name = "ui", .clear = false,
//                             .features = { { &r2d, view_bit(View::Screen) } } });
//   core.set_recipe(std::move(recipe));
//
// Color spaces: the scene is sRGB-encoded (the art as drawn); the composite
// decodes it to linear and multiplies it by (ambient + light), both linear.
// Everything after (emissive draws, bloom) is linear HDR. The tonemap maps
// it into 0..1 and encodes sRGB for the backbuffer. So daylight (ambient 1,
// no lights) shows the art exactly as drawn, with Tonemap::Shoulder.
//
// Bloom (Jimenez, "Next Generation Post Processing in Call of Duty: Advanced
// Warfare", 2014): a chain of half-size targets, each a 13-tap downsample of
// the one above (the first with a soft threshold and Karis averaging against
// fireflies), then a tent-filter upsample added back up the chain. Cheap: the
// biggest level is a quarter of the screen's pixels.
//
// GPU work: one fullscreen pass for the composite and one for the tonemap,
// plus 2 * bloom_levels - 1 small ones.
// =============================================================================

#pragma once
#include "Nezumi/Render/Core/Frame/IRenderFeature.hpp"
#include "Nezumi/Render/Core/Frame/PassConfig.hpp"
#include "Nezumi/Render/Core/Frame/RenderTarget.hpp"
#include "Nezumi/Render/Core/Post/PostProcessConfig.hpp"
#include "Nezumi/RHI/RHI.hpp"
#include <cstdint>     // uint32_t
#include <string_view> // string_view
#include <vector>      // vector

namespace Nezumi::Render {

class RenderCore;

class PostProcess final : public IRenderFeature {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    // Registers with `core` and creates its targets (lit + bloom chain);
    // the destructor unregisters. Throws if a shader or pipeline fails.
    PostProcess(RenderCore& core, const PostProcessConfig& config);
    ~PostProcess() override;

    // Non-copyable and non-movable
    PostProcess(const PostProcess&)            = delete;
    PostProcess& operator=(const PostProcess&) = delete;
    PostProcess(PostProcess&&)                 = delete;
    PostProcess& operator=(PostProcess&&)      = delete;

    // -------------------------------------------------------------------------
    // Frame Hooks (IRenderFeature)
    // -------------------------------------------------------------------------

    std::string_view name() const override { return "post-process"; }

    void upload(RenderCore& core) override;
    void record(RenderCore& core, RHI::CommandEncoder& encoder, const PassContext& pass) override;

    // -------------------------------------------------------------------------
    // Passes
    //
    // Put composite_pass() after the scene and light passes, then any
    // emissive pass into lit_target(), then output_passes(), then the UI.
    // -------------------------------------------------------------------------

    // lit = scene x (ambient + light), into lit_target()
    PassConfig composite_pass() const;

    // Bloom downsamples and upsamples, then the tonemap into `output`
    std::vector<PassConfig> output_passes(TargetID output = Target::Backbuffer) const;

    // The linear HDR image: the composite writes it, emissive draws add to
    // it (straight alpha blending, linear colors, may exceed 1)
    TargetID lit_target() const { return lit_; }

    // -------------------------------------------------------------------------
    // Settings
    //
    // Read once per frame, in upload().
    // -------------------------------------------------------------------------

    PostSettings&       settings()       { return settings_; }
    const PostSettings& settings() const { return settings_; }

private:

    // -------------------------------------------------------------------------
    // Steps
    // -------------------------------------------------------------------------

    enum class StepKind : std::uint8_t { Composite, Down, Up, Tonemap };

    struct Step {
        StepKind      kind    = StepKind::Composite;
        std::uint32_t level   = 0; // Down / Up: bloom level written
        TargetID      src0    = Target::Backbuffer;
        TargetID      src1    = Target::Backbuffer;
        TargetID      dst     = Target::Backbuffer; // Tonemap: set per pass

        RHI::BindGroupHandle group = {};
        RHI::TextureHandle   tex0  = {}; // what `group` was built over
        RHI::TextureHandle   tex1  = {};
    };

    // Steps by index = PassFeature::step
    std::uint32_t composite_step_() const { return 0; }
    std::uint32_t down_step_(std::uint32_t level) const { return 1 + level; }
    std::uint32_t up_step_  (std::uint32_t level) const { return 1 + levels_ + level; }
    std::uint32_t tonemap_step_() const { return 1 + 2 * levels_; }

    RHI::RenderPipelineHandle make_pipeline_(
        std::string_view       name,
        std::string_view       frag_path,
        RHI::Format            format,
        const RHI::BlendState& blend);

    RHI::RenderPipelineHandle tonemap_pipeline_(RHI::Format format);

    void refresh_group_(Step& step, std::uint32_t index);

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RenderCore&       core_;
    PostProcessConfig config_;
    PostSettings      settings_;

    std::uint32_t         levels_ = 0;
    TargetID              lit_    = Target::Backbuffer;
    std::vector<TargetID> bloom_; // level 0 = half the lit target
    std::vector<Step>     steps_;

    RHI::BindGroupLayoutHandle layout_   = {};
    RHI::SamplerHandle         sampler_  = {};
    RHI::BufferHandle          uniforms_ = {}; // one 256-byte slot per step
    RHI::ShaderHandle          vert_     = {};

    RHI::RenderPipelineHandle composite_ = {};
    RHI::RenderPipelineHandle down_      = {};
    RHI::RenderPipelineHandle up_        = {};

    struct TonemapPipeline {
        RHI::Format               format   = RHI::Format::Undefined;
        RHI::RenderPipelineHandle pipeline = {};
    };
    std::vector<TonemapPipeline> tonemaps_; // per output format
    RHI::Format                  output_format_ = RHI::Format::Undefined; // the last output_passes()
};

} // namespace Nezumi::Render
