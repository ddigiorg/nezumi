// =============================================================================
// Renderer2D.cpp
// =============================================================================

#include "Nezumi/Render/2D/Renderer2D.hpp"
#include "Nezumi/Render/Core/View/ViewID.hpp"
#include <cassert> // assert

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

Renderer2D::Renderer2D(RenderCore& core, const Renderer2DConfig& config)
    : core_  (core)
    , config_(config)
    , queue_ (config.reserve_arena_bytes, config.reserve_commands)
    , kinds_ (core.device())
{
    // Setup default cameras for the default views
    views_[View::World] = cameras_.create(
        CameraConfig{
            .name            = "world-camera",
            .origin          = CameraOrigin::Centered,
            .pixels_per_unit = config.world_pixels_per_unit,
        }
    ).handle;

    views_[View::Screen] = cameras_.create(
        CameraConfig{
            .name   = "screen-camera",
            .origin = CameraOrigin::BottomLeft,
        }
    ).handle;

    // Register a kind's pipeline now instead of on its first draw time so a
    // broken shader fails early and the first frame doesn't hitch.
    RenderContext& ctx = core_.context();
    kinds_.ensure<ArrowKind            >(ctx);
    kinds_.ensure<CircleKind           >(ctx);
    kinds_.ensure<LineKind             >(ctx);
    kinds_.ensure<QuadKind             >(ctx);
    kinds_.ensure<RectangleFilledKind  >(ctx);
    kinds_.ensure<RectangleUnfilledKind>(ctx);
    kinds_.ensure<RectangleSDFKind     >(ctx);
    kinds_.ensure<SpriteKind           >(ctx);
    kinds_.ensure<TextGlyphSDFKind     >(ctx);
    kinds_.ensure<UIQuadKind           >(ctx);

    core_.add_feature(*this);
}

Renderer2D::~Renderer2D() {
    core_.remove_feature(*this);
}

// -----------------------------------------------------------------------------
// Frame Hooks
// -----------------------------------------------------------------------------

void Renderer2D::begin_frame(RenderCore& core) {
    queue_.clear();
    batches_ = {};

    // Size and scale each view's camera to the backbuffer here as well as in
    // upload(), so draw code sees the real screen size on the first frame
    // (cameras start at CameraConfig::size) and the frame after a resize.
    const RHI::Extent2D size = core.device().backbuffer_extent();
    if (size.width > 0 && size.height > 0) {
        for (ViewID vid = 0; vid < VIEW_CAPACITY; vid++) {
            if (Camera2D* camera = cameras_.get(views_[vid])) {
                camera->set_size({ float(size.width), float(size.height) });
                camera->set_display_scale(core.get_display_scale());
            }
        }
    }
}

void Renderer2D::prepare(RenderCore& core) {
    // CPU: sort + merge + gather into the stream mirrors.
    batches_ = builder_.build(queue_, core.stream());

    stats_.pipeline_switches = 0;
    stats_.material_switches = 0;
    stats_.view_switches     = 0;
    stats_.dropped           = 0;
}

void Renderer2D::upload(RenderCore& core) {
    // Every view with a live camera: size it to the backbuffer (final size
    // for this frame, now that the device frame has begun) and publish its
    // matrices to the view's slot.
    const RHI::Extent2D size = core.device().backbuffer_extent();
    for (ViewID vid = 0; vid < VIEW_CAPACITY; vid++) {
        if (Camera2D* camera = cameras_.get(views_[vid])) {
            camera->set_size({ float(size.width), float(size.height) });
            camera->set_display_scale(core.get_display_scale());
            core.set_view(vid, camera->get_view_data());
        }
    }
}

void Renderer2D::record(
    RenderCore&          core,
    RHI::CommandEncoder& encoder,
    ViewMask             views)
{
    ViewID     bound_view     = 0xFF; // nothing bound yet
    bool       have_kind      = false;
    KindID     bound_kind     = 0;
    MaterialID bound_material = Material::None;

    for (const DrawBatch& batch : batches_) {

        // Skip views this pass doesn't record. Batches arrive grouped by view
        // (its bits lead the sort key).
        if (!(views & view_bit(batch.view))) continue;

        // Resolve batch view change: binds group 0 once per view per pass.
        if (batch.view != bound_view) {
            if (!cameras_.get(views_[batch.view])) { // view lost its camera
                stats_.dropped += batch.instance_count;
                continue;
            }
            core.views().bind(encoder, batch.view);
            bound_view     = batch.view;
            bound_material = Material::None; // cheap insurance: rebind group 1 too
            stats_.view_switches++;
        }

        const KindResources& kind = kinds_.get(batch.kind);

        // Resolve batch kind change
        if (!have_kind || batch.kind != bound_kind) {
            encoder.set_render_pipeline(
                /*.pipeline = */kind.pipeline
            );
            encoder.set_vertex_buffer(
                /*.slot   = */0,
                /*.buffer = */kind.shape
            );
            bound_kind = batch.kind;
            have_kind  = true;
            stats_.pipeline_switches++;

            // Group 1 only survives a pipeline switch if both pipelines
            // declare it with the same layout, and untextured kinds don't
            // declare it at all. Simplest correct rule: rebind after any
            // switch. (Costs one extra bind per kind change, not per batch.)
            bound_material = Material::None;
        }

        // Resolve batch material change
        if (batch.material != bound_material && batch.material != Material::None) {
            core.materials().bind(encoder, batch.material);
            bound_material = batch.material;
            stats_.material_switches++;
        }

        encoder.set_vertex_buffer(
            /*.slot   = */1,
            /*.buffer = */batch.buffer,
            /*.offset = */batch.byte_offset
        );

        encoder.draw(kind.vertex_count, batch.instance_count);
    }
}

void Renderer2D::finish(RenderCore& core) {
    const BatchBuilder::Stats& b = builder_.stats();
    stats_.objects       = b.objects;
    stats_.draw_calls    = b.batches;
    stats_.dropped      += b.dropped; // + draws to views without a camera (record)
    stats_.stream_bytes  = core.stream().bytes_used();
    stats_.stream_chunks = core.stream().chunk_count();
}

// -----------------------------------------------------------------------------
// Create
// -----------------------------------------------------------------------------

SlotMap<Atlas>::Pair Renderer2D::create_atlas(const AtlasConfig& config) {
    return atlases_.create(core_.context(), config);
}

SlotMap<Camera2D>::Pair Renderer2D::create_camera(const CameraConfig& config) {
    return cameras_.create(config);
}

SlotMap<Font>::Pair Renderer2D::create_font(const FontConfig& config) {
    return fonts_.create(core_.context(), config);
}

// -----------------------------------------------------------------------------
// Remove
// -----------------------------------------------------------------------------

void Renderer2D::remove_atlas(Handle<Atlas> handle) {
    atlases_.remove(handle);
}

void Renderer2D::remove_camera(Handle<Camera2D> handle) {
    cameras_.remove(handle);
}

void Renderer2D::remove_font(Handle<Font> handle) {
    fonts_.remove(handle);
}

// -----------------------------------------------------------------------------
// Get
// -----------------------------------------------------------------------------

Atlas* Renderer2D::get_atlas(Handle<Atlas> handle) {
    return atlases_.get(handle);
}

Camera2D* Renderer2D::get_camera(Handle<Camera2D> handle) {
    return cameras_.get(handle);
}

Font* Renderer2D::get_font(Handle<Font> handle) {
    return fonts_.get(handle);
}

// -----------------------------------------------------------------------------
// Views
// -----------------------------------------------------------------------------

void Renderer2D::set_view(ViewID view, Handle<Camera2D> camera) {
    assert(view < VIEW_CAPACITY && "Renderer2D: setting view out of range");
    views_[view] = camera;
}

void Renderer2D::clear_view(ViewID view) {
    assert(view < VIEW_CAPACITY && "Renderer2D: clearing view out of range");
    views_[view] = {};
}

Handle<Camera2D> Renderer2D::get_view(ViewID view) const {
    return view < VIEW_CAPACITY ? views_[view] : Handle<Camera2D>{};
}

Camera2D* Renderer2D::get_view_camera(ViewID view) {
    return cameras_.get(get_view(view));
}

// -----------------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------------

// Render arrow render object
void Renderer2D::draw_arrow(
    const Arrow&      arrow,
    const DrawParams& params)
{
    draw<ArrowKind>(arrow, params);
}

// Render Circle render object
void Renderer2D::draw_circle(
    const Circle&     circle,
    const DrawParams& params,
    const bool        filled)
{
    // The shader fills the whole disc once width reaches radius.
    Circle instance = circle;
    if (filled) instance.width = instance.radius;
    draw<CircleKind>(instance, params);
}

// Render Line render object
void Renderer2D::draw_line(
    const Line&       line,
    const DrawParams& params)
{
    draw<LineKind>(line, params);
}

// Render Quad render object
void Renderer2D::draw_quad(
    const Quad&       quad,
    const DrawParams& params)
{
    assert(params.material != Material::None &&
           "Renderer2D: draw_quad needs a material (e.g. atlas->material())");
    draw<QuadKind>(quad, params);
}

// Render Rectangle render object
void Renderer2D::draw_rectangle(
    const Rectangle&  rectangle,
    const DrawParams& params,
    const bool        filled)
{
    (filled)
        ? draw<RectangleFilledKind  >(rectangle, params)
        : draw<RectangleUnfilledKind>(rectangle, params);
}

// Render RectangleSDF render object
void Renderer2D::draw_rectangle_sdf(
    const RectangleSDF& rectangle,
    const DrawParams&   params)
{
    draw<RectangleSDFKind>(rectangle, params);
}

// Render Sprite render object
void Renderer2D::draw_sprite(
    const Sprite&     sprite,
    const DrawParams& params)
{
    assert(params.material != Material::None &&
           "Renderer2D: draw_sprite needs a material (e.g. atlas->material())");
    draw<SpriteKind>(sprite, params);
}

// Render text using TextGlyph render objects
void Renderer2D::draw_text(
    std::string_view text,
    glm::vec2        position,
    const Font&      font,
    const TextStyle& style,
    DrawParams       params)
{
    if (text.empty() || font.material() == Material::None) return;
    params.material = font.material();

    // Style -> shader units, once per string. One atlas pixel of distance is
    // 1 / (2 * spread) in SDF value units; offsets become UVs (y up -> v down).
    const float     per_px = 1.0f / (2.0f * font.sdf_spread());
    const glm::vec4 shader_params = {
        style.outline_width * per_px,
        style.weight        * per_px,
        style.outline_offset.x / float(font.atlas_width()),
       -style.outline_offset.y / float(font.atlas_height()),
    };

    const auto draw_pass = [&](const glm::vec4& color, const glm::vec4& outline_color) {
        layout_text(font, text, position, style, [&](const FontGlyph& g, const glm::vec4& box) {
            draw<TextGlyphSDFKind>(TextGlyphSDF{
                .pos_box       = box,
                .tex_box       = g.tex_box,
                .color         = color,
                .outline_color = outline_color,
                .params        = shader_params,
            }, params);
        });
    };

    const glm::vec4 none = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (style.has_outline()) {
        // All outlines first, then all fills: same sort key, so submission
        // order holds and no glyph's outline lands on its neighbour's fill.
        // Still one draw call; twice the glyph instances.
        draw_pass(none, style.outline_color);
        draw_pass(style.color, none);
    } else {
        draw_pass(style.color, none);
    }
}

} // namespace Nezumi::Render
