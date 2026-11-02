// =============================================================================
// PostProcess.cpp
// =============================================================================

#include "Nezumi/Render/Core/Post/PostProcess.hpp"

#include "Nezumi/Render/Core/RenderCore.hpp"
#include "Nezumi/Render/Core/Shader/ShaderLibrary.hpp"
#include "Nezumi/RHI/Common/FormatInfo.hpp"
#include <algorithm>   // max, min
#include <array>       // array
#include <cassert>     // assert
#include <cstddef>     // byte
#include <glm/glm.hpp> // vec4
#include <stdexcept>   // runtime_error
#include <string>      // string, to_string

namespace Nezumi::Render {

namespace {

constexpr std::string_view VERT_SHADER      = "assets/shaders/post.vert.glsl";
constexpr std::string_view COMPOSITE_SHADER = "assets/shaders/post-composite.frag.glsl";
constexpr std::string_view DOWN_SHADER      = "assets/shaders/post-bloom-down.frag.glsl";
constexpr std::string_view UP_SHADER        = "assets/shaders/post-bloom-up.frag.glsl";
constexpr std::string_view TONEMAP_SHADER   = "assets/shaders/post-tonemap.frag.glsl";

constexpr std::uint32_t MAX_BLOOM_LEVELS = 8;

// One std140 block per step: 4 vec4s, in a 256-byte slot (the largest
// uniform offset alignment devices ask for)
struct StepUniforms {
    glm::vec4 p0 = {};
    glm::vec4 p1 = {};
    glm::vec4 p2 = {};
    glm::vec4 p3 = {};
};
constexpr std::uint64_t SLOT_SIZE = 256;
static_assert(sizeof(StepUniforms) <= SLOT_SIZE);

} // namespace

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

PostProcess::PostProcess(RenderCore& core, const PostProcessConfig& config)
    : core_  (core)
    , config_(config)
    , levels_(std::min(config.bloom_levels, MAX_BLOOM_LEVELS))
{
    if (config_.scene == Target::Backbuffer) {
        throw std::runtime_error("PostProcess: needs a scene target (RenderCore::create_target)");
    }

    RHI::Device& device = core_.device();

    // Targets: the lit image, and the bloom chain below it
    lit_ = core_.create_target({ .name = "post-lit", .format = config_.lit_format, .scale = 1.0f });

    const float lit_scale = 1.0f;
    for (std::uint32_t i = 0; i < levels_; i++) {
        const float scale = lit_scale / float(2u << i); // 1/2, 1/4, ...
        bloom_.push_back(core_.create_target({
            .name   = "post-bloom-" + std::to_string(i),
            .format = config_.lit_format,
            .scale  = scale,
        }));
    }

    // Steps, by PassFeature::step: composite, downs, ups, tonemap
    const TargetID light = config_.light != Target::Backbuffer ? config_.light : config_.scene;
    steps_.push_back({ .kind = StepKind::Composite, .src0 = config_.scene, .src1 = light, .dst = lit_ });
    for (std::uint32_t i = 0; i < levels_; i++) {
        const TargetID src = i == 0 ? lit_ : bloom_[i - 1];
        steps_.push_back({ .kind = StepKind::Down, .level = i, .src0 = src, .src1 = src, .dst = bloom_[i] });
    }
    for (std::uint32_t i = 0; i < levels_; i++) {
        // Up level i adds level i + 1 into level i; the last level has none
        const TargetID src = i + 1 < levels_ ? bloom_[i + 1] : bloom_[i];
        steps_.push_back({ .kind = StepKind::Up, .level = i, .src0 = src, .src1 = src, .dst = bloom_[i] });
    }
    const TargetID bloom0 = levels_ > 0 ? bloom_[0] : lit_;
    steps_.push_back({ .kind = StepKind::Tonemap, .src0 = lit_, .src1 = bloom0 });

    // Bind group layout (group 0): step uniforms, two textures, one sampler
    const std::array<RHI::BindGroupLayoutEntry, 4> entries = {
        RHI::BindGroupLayoutEntry{
            .binding = 0,
            .type    = RHI::BindType::UniformBuffer,
            .stage   = RHI::ShaderStageFlags::Fragment,
            .name    = "Post",
        },
        RHI::BindGroupLayoutEntry{
            .binding = 1,
            .type    = RHI::BindType::SampledTexture,
            .stage   = RHI::ShaderStageFlags::Fragment,
            .name    = "u_tex0",
        },
        RHI::BindGroupLayoutEntry{
            .binding = 2,
            .type    = RHI::BindType::SampledTexture,
            .stage   = RHI::ShaderStageFlags::Fragment,
            .name    = "u_tex1",
        },
        RHI::BindGroupLayoutEntry{
            .binding = 3,
            .type    = RHI::BindType::Sampler,
            .stage   = RHI::ShaderStageFlags::Fragment,
        },
    };
    layout_ = device.create(RHI::BindGroupLayoutConfig{ .name = "post", .entries = entries });

    // Bilinear, clamped: the light target and the bloom levels are smaller
    // than what reads them
    sampler_ = device.create(RHI::SamplerConfig{ .name = "post" });

    uniforms_ = device.create(RHI::BufferConfig{
        .name  = "post-uniforms",
        .size  = SLOT_SIZE * steps_.size(),
        .usage = RHI::BufferUsageFlags::Uniform | RHI::BufferUsageFlags::CopyDst,
    });

    if (!layout_ || !sampler_ || !uniforms_) {
        throw std::runtime_error("PostProcess: bind group layout, sampler or buffer failed (see log)");
    }

    vert_ = core_.shaders().get(VERT_SHADER, RHI::ShaderStageFlags::Vertex, RHI::ShaderFormat::Glsl);
    if (!vert_) throw std::runtime_error("PostProcess: vertex shader failed (see log)");

    composite_ = make_pipeline_("post-composite",  COMPOSITE_SHADER, config_.lit_format, {});
    down_      = make_pipeline_("post-bloom-down", DOWN_SHADER,      config_.lit_format, {});
    up_        = make_pipeline_("post-bloom-up",   UP_SHADER,        config_.lit_format, RHI::BlendState::ADD());

    core_.add_feature(*this);
}

PostProcess::~PostProcess() {
    core_.remove_feature(*this);

    RHI::Device& device = core_.device();
    for (Step& step : steps_) device.remove(step.group);
    for (TonemapPipeline& t : tonemaps_) device.remove(t.pipeline);
    device.remove(composite_);
    device.remove(down_);
    device.remove(up_);
    device.remove(uniforms_);
    device.remove(sampler_);
    device.remove(layout_);
    // Targets belong to RenderCore, and the shader to its library
}

// -----------------------------------------------------------------------------
// Passes
// -----------------------------------------------------------------------------

PassConfig PostProcess::composite_pass() const {
    return PassConfig{
        .name     = "post-composite",
        .target   = lit_,
        .clear    = false, // every pixel is written
        .features = { { .feature = const_cast<PostProcess*>(this), .step = composite_step_() } },
    };
}

std::vector<PassConfig> PostProcess::output_passes(TargetID output) const {
    PostProcess* self = const_cast<PostProcess*>(this);
    std::vector<PassConfig> passes;

    // Down the chain: each level cleared, then written whole
    for (std::uint32_t i = 0; i < levels_; i++) {
        passes.push_back({
            .name     = "post-bloom-down-" + std::to_string(i),
            .target   = bloom_[i],
            .clear    = false,
            .features = { { .feature = self, .step = down_step_(i) } },
        });
    }

    // Back up: each level adds the blurred level below it onto itself
    for (std::uint32_t n = levels_; n-- > 1; ) {
        const std::uint32_t i = n - 1;
        passes.push_back({
            .name     = "post-bloom-up-" + std::to_string(i),
            .target   = bloom_[i],
            .clear    = false, // keep the downsample: the upsample adds to it
            .features = { { .feature = self, .step = up_step_(i) } },
        });
    }

    passes.push_back({
        .name     = "post-tonemap",
        .target   = output,
        .clear    = false,
        .features = { { .feature = self, .step = tonemap_step_() } },
    });

    // The tonemap's pipeline for this output's format, made now so recording
    // never creates one. An sRGB output encodes on write by itself; any
    // other gets sRGB-encoded in the shader (upload passes the flag).
    self->output_format_ = core_.target_format(output);
    self->tonemap_pipeline_(self->output_format_);

    return passes;
}

// -----------------------------------------------------------------------------
// Frame Hooks
// -----------------------------------------------------------------------------

void PostProcess::upload(RenderCore& core) {
    const PostSettings& s = settings_;

    std::vector<StepUniforms> u(steps_.size());

    // Composite: p0 = ambient, lighting on; p1.x = show the light alone
    u[composite_step_()].p0 = { s.ambient, s.lighting ? 1.0f : 0.0f };
    u[composite_step_()].p1 = { s.debug == PostDebugView::Light ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };

    // Down: p0 = threshold, knee, first level (prefilter)
    for (std::uint32_t i = 0; i < levels_; i++) {
        u[down_step_(i)].p0 = { s.bloom_threshold, std::max(s.bloom_knee, 1e-4f), i == 0 ? 1.0f : 0.0f, 0.0f };
    }

    // Up: p0.x = filter radius in source texels
    for (std::uint32_t i = 0; i < levels_; i++) {
        u[up_step_(i)].p0 = { std::max(s.bloom_radius, 0.0f), 0.0f, 0.0f, 0.0f };
    }

    // Tonemap: p0 = exposure, bloom strength, tonemap, show bloom alone;
    // p1 = shoulder, encode sRGB. Level 0 holds the sum of every level, so the
    // strength is shared out over them.
    const bool  bloom    = s.bloom && levels_ > 0;
    const float strength = bloom ? s.bloom_intensity / float(levels_) : 0.0f;
    const float show_bloom = (bloom && s.debug == PostDebugView::Bloom) ? 1.0f : 0.0f;
    u[tonemap_step_()].p0 = { s.exposure, strength, float(static_cast<std::uint8_t>(s.tonemap)), show_bloom };
    const float encode_srgb = RHI::is_srgb(output_format_) ? 0.0f : 1.0f;
    u[tonemap_step_()].p1 = { std::clamp(s.shoulder, 0.0f, 0.99f), encode_srgb, 0.0f, 0.0f };

    // One write per slot
    for (std::uint32_t i = 0; i < steps_.size(); i++) {
        core.device().write_buffer(uniforms_, SLOT_SIZE * i, std::as_bytes(std::span(&u[i], 1)));
    }

    // Targets may have been recreated (resize): rebuild what pointed at them
    for (std::uint32_t i = 0; i < steps_.size(); i++) refresh_group_(steps_[i], i);
}

void PostProcess::record(
    RenderCore&          /*core*/,
    RHI::CommandEncoder& encoder,
    const PassContext&   pass)
{
    if (pass.step >= steps_.size()) return;
    const Step& step = steps_[pass.step];

    RHI::RenderPipelineHandle pipeline = {};
    switch (step.kind) {
        case StepKind::Composite: pipeline = composite_; break;
        case StepKind::Down:      pipeline = settings_.bloom ? down_ : RHI::RenderPipelineHandle{}; break;
        case StepKind::Up:
            // The last level has nothing below it to add
            pipeline = (settings_.bloom && step.level + 1 < levels_) ? up_ : RHI::RenderPipelineHandle{};
            break;
        case StepKind::Tonemap:
            for (const TonemapPipeline& t : tonemaps_) {
                if (t.format == pass.format) pipeline = t.pipeline;
            }
            break;
    }
    if (!pipeline || !step.group) return;

    encoder.set_render_pipeline(pipeline);
    encoder.set_bind_group(0, step.group);
    encoder.draw(3, 1); // one triangle over the whole target
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

RHI::RenderPipelineHandle PostProcess::make_pipeline_(
    std::string_view       name,
    std::string_view       frag_path,
    RHI::Format            format,
    const RHI::BlendState& blend)
{
    const RHI::ShaderHandle frag = core_.shaders().get(
        frag_path, RHI::ShaderStageFlags::Fragment, RHI::ShaderFormat::Glsl);
    if (!frag) {
        throw std::runtime_error("PostProcess: shader '" + std::string(frag_path) + "' failed (see log)");
    }

    const std::array<RHI::BindGroupLayoutHandle, 1> layouts = { layout_ };
    const std::array<RHI::ColorTargetState, 1> color_targets = {
        RHI::ColorTargetState{ .format = format, .blend = blend },
    };

    const RHI::RenderPipelineHandle pipeline = core_.device().create(
        RHI::RenderPipelineConfig{
            .name               = name,
            .vertex_shader      = vert_,
            .fragment_shader    = frag,
            .vertex_buffers     = {}, // the vertex shader makes the triangle
            .bind_group_layouts = layouts,
            .primitive          = { .topology = RHI::PrimitiveTopology::TriangleList },
            .color_targets      = color_targets,
        }
    );
    if (!pipeline) {
        throw std::runtime_error("PostProcess: pipeline '" + std::string(name) + "' failed (see log)");
    }
    return pipeline;
}

RHI::RenderPipelineHandle PostProcess::tonemap_pipeline_(RHI::Format format) {
    for (const TonemapPipeline& t : tonemaps_) {
        if (t.format == format) return t.pipeline;
    }

    const RHI::RenderPipelineHandle pipeline = make_pipeline_("post-tonemap", TONEMAP_SHADER, format, {});
    tonemaps_.push_back({ format, pipeline });
    return pipeline;
}

void PostProcess::refresh_group_(Step& step, std::uint32_t index) {
    const RHI::TextureHandle tex0 = core_.target_texture(step.src0);
    const RHI::TextureHandle tex1 = core_.target_texture(step.src1);
    if (step.group && tex0 == step.tex0 && tex1 == step.tex1) return;

    RHI::Device& device = core_.device();
    device.remove(step.group);

    const std::array<RHI::BindGroupEntry, 4> entries = {
        RHI::BindGroupEntry{ .binding = 0, .buffer = uniforms_, .offset = SLOT_SIZE * index,
                             .size = sizeof(StepUniforms) },
        RHI::BindGroupEntry{ .binding = 1, .texture = tex0 },
        RHI::BindGroupEntry{ .binding = 2, .texture = tex1 },
        RHI::BindGroupEntry{ .binding = 3, .sampler = sampler_ },
    };

    step.group = device.create(RHI::BindGroupConfig{
        .name    = "post-step",
        .layout  = layout_,
        .entries = entries,
    });
    step.tex0 = tex0;
    step.tex1 = tex1;
}

} // namespace Nezumi::Render
