// =============================================================================
// Renderer.cpp
// =============================================================================

#include "Nezumi/Render/Renderer.hpp"
#include <cassert> // assert

namespace Nezumi::Render {

// -----------------------------------------------------------------------------
// Construction & Destruction
// -----------------------------------------------------------------------------

Renderer::Renderer(RHI::Device& device, const RendererConfig& config)
    : device_   (device)
    , config_   (config)
    , frame_    (device)
    , queue_    (config.reserve_arena_bytes, config.reserve_commands)
    , stream_   (device, config.stream_chunk_size)
    , materials_(device)
    , shaders_  (device)
    , kinds_    (device)
{
    // Setup default views and their respective cameras

    views_[View::Player] = cameras_.create(
        CameraConfig{
            .name   = "player-camera",
            .origin = CameraOrigin::Centered
        }
    ).handle;
    views_[View::Screen] = cameras_.create(
        CameraConfig{
            .name   = "screen-camera",
            .origin = Render::CameraOrigin::BottomLeft
        }
    ).handle;

    // Setup context
    context_ = {
        .device    = &device_,
        .config    = &config_,
        .frame     = &frame_,
        .materials = &materials_,
        .shaders   = &shaders_,
        .kinds     = &kinds_,
        .stream    = &stream_,
        .queue     = &queue_,
        .builder   = &builder_,
    };

    // Register a kind's pipeline now instead of on its first draw time so a
    // broken shader fails early and the first frame doesn't hitch.
    kinds_.ensure<ArrowKind            >(context_);
    kinds_.ensure<CircleKind           >(context_);
    kinds_.ensure<LineKind             >(context_);
    kinds_.ensure<RectangleFilledKind  >(context_);
    kinds_.ensure<RectangleUnfilledKind>(context_);
    kinds_.ensure<RectangleSDFKind     >(context_);
    kinds_.ensure<SpriteKind           >(context_);
    kinds_.ensure<TextGlyphSDFKind     >(context_);
}

// -----------------------------------------------------------------------------
// Frame Control
// -----------------------------------------------------------------------------

void Renderer::begin_frame() {
    assert(!in_frame_ && "Renderer: begin_frame called twice");
    queue_.clear();
    stream_.reset();
    in_frame_ = true;
}

void Renderer::end_frame() {
    assert(in_frame_ && "Renderer: end_frame without begin_frame");
    in_frame_ = false;

    // -------------------------------------------------------------------------
    // Step 1
    //
    // CPU: sort + merge + gather into the stream mirrors. Done before
    // device_.begin_frame(), which may block on an older frame, so this work
    // overlaps with the GPU.
    // -------------------------------------------------------------------------

    const std::span<const DrawBatch> batches = builder_.build(queue_, stream_);

    // -------------------------------------------------------------------------
    // Step 2
    //
    // Upload data from CPU to the device. Inside the device frame, writes are
    // staged and all land before any of this frame's commands run.
    // -------------------------------------------------------------------------

    RHI::CommandEncoder& encoder = device_.begin_frame();

    // Every view with a live camera: size it to the backbuffer and upload its
    // matrix to the view's slot (each slot is its own byte range).
    const RHI::Extent2D size = device_.backbuffer_extent();
    for (ViewID view = 0; view < VIEW_COUNT; ++view) {
        if (Camera* camera = cameras_.get(views_[view])) {
            camera->set_size({ float(size.width), float(size.height) });
            frame_.update(view, camera->get_vp());
        }
    }

    stream_.flush();

    // -------------------------------------------------------------------------
    // Step 3
    //
    // Record and execute commands on the device.
    // -------------------------------------------------------------------------

    const RHI::ColorAttachment clear_color_attachment[] = {{
        .texture     = device_.backbuffer(),
        .load_op     = RHI::LoadOp::Clear,
        .store_op    = RHI::StoreOp::Store,
        .clear_color = config_.clear_color,
    }};

    encoder.begin_render_pass(
        RHI::RenderPassConfig{
            .name              = "main",
            .color_attachments = clear_color_attachment,
        }
    );

    stats_.pipeline_switches = 0;
    stats_.material_switches = 0;
    stats_.view_switches     = 0;
    stats_.dropped           = 0;

    ViewID     bound_view     = 0xFF; // nothing bound yet
    bool       have_kind      = false;
    KindID     bound_kind     = 0;
    MaterialID bound_material = Material::None;

    for (const DrawBatch& batch : batches) {

        // Resolve batch view change. Batches arrive grouped by view (its bits
        // lead the sort key), so this binds group 0 once per view per frame.
        if (batch.view != bound_view) {
            if (!cameras_.get(views_[batch.view])) { // view lost its camera
                stats_.dropped += batch.instance_count;
                continue;
            }
            frame_.bind(encoder, batch.view);
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
            materials_.bind(encoder, batch.material);
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

    encoder.end_render_pass();
    device_.end_frame();

    // -------------------------------------------------------------------------
    // Step 4
    //
    // Record frame statistics.
    // -------------------------------------------------------------------------

    const BatchBuilder::Stats& b = builder_.stats();
    stats_.objects       = b.objects;
    stats_.draw_calls    = b.batches;
    stats_.dropped      += b.dropped; // + draws to views without a camera (above)
    stats_.stream_bytes  = stream_.bytes_used();
    stats_.stream_chunks = stream_.chunk_count();
}

// -----------------------------------------------------------------------------
// Create
// -----------------------------------------------------------------------------

SlotMap<Atlas>::Pair Renderer::create_atlas(const AtlasConfig& config) {
    return atlases_.create(context_, config);
}

SlotMap<Camera>::Pair Renderer::create_camera(const CameraConfig& config) {
    return cameras_.create(config);
}

SlotMap<Font>::Pair Renderer::create_font(const FontConfig& config) {
    return fonts_.create(context_, config);
}

// -----------------------------------------------------------------------------
// Remove
// -----------------------------------------------------------------------------

void Renderer::remove_atlas(Handle<Atlas> handle) {
    atlases_.remove(handle);
}

void Renderer::remove_camera(Handle<Camera> handle) {
    cameras_.remove(handle);
}

void Renderer::remove_font(Handle<Font> handle) {
    fonts_.remove(handle);
}

// -----------------------------------------------------------------------------
// Get
// -----------------------------------------------------------------------------

Atlas* Renderer::get_atlas(Handle<Atlas> handle) {
    return atlases_.get(handle);
}

Camera* Renderer::get_camera(Handle<Camera> handle) {
    return cameras_.get(handle);
}

Font* Renderer::get_font(Handle<Font> handle) {
    return fonts_.get(handle);
}

// -----------------------------------------------------------------------------
// Views
// -----------------------------------------------------------------------------

void Renderer::set_view(ViewID view, Handle<Camera> camera) {
    assert(view < VIEW_COUNT && "Renderer: setting view out of range");
    views_[view] = camera;
}

void Renderer::clear_view(ViewID view) {
    assert(view < VIEW_COUNT && "Renderer: clearing view out of range");
    views_[view] = {};
}

Handle<Camera> Renderer::get_view(ViewID view) const {
    return view < VIEW_COUNT ? views_[view] : Handle<Camera>{};
}

Camera* Renderer::get_player_camera() {
    return cameras_.get(views_[View::Player]);
}

Camera* Renderer::get_screen_camera() {
    return cameras_.get(views_[View::Screen]);
}

// -----------------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------------

// Render arrow render object
void Renderer::draw_arrow(
    const Arrow&      arrow,
    const DrawParams& params)
{
    draw<ArrowKind>(arrow, params);
}

// Render Circle render object
void Renderer::draw_circle(
    const Circle&     circle,
    const DrawParams& params,
    const bool        filled)
{
    Circle new_circle = circle; // TODO: could probably be better

    if (filled) {
        new_circle.width = new_circle.radius;
    } else {
        assert(circle.width < circle.radius); // TODO: needs message
    }
    draw<CircleKind>(new_circle, params);
}

// Render Line render object
void Renderer::draw_line(
    const Line&       line,
    const DrawParams& params)
{
    draw<LineKind>(line, params);
}

// Render Rectangle render object
void Renderer::draw_rectangle(
    const Rectangle&  rectangle,
    const DrawParams& params,
    const bool        filled)
{
    (filled)
        ? draw<RectangleFilledKind  >(rectangle, params)
        : draw<RectangleUnfilledKind>(rectangle, params);
}

// Render RectangleSDF render object
void Renderer::draw_rectangle_sdf(
    const RectangleSDF& rectangle,
    const DrawParams&   params)
{
    draw<RectangleSDFKind>(rectangle, params);
}

// Render Sprite render object
void Renderer::draw_sprite(
    const Sprite&     sprite,
    const DrawParams& params)
{
    draw<SpriteKind>(sprite, params);
}

// Render text using TextGlyph render objects
void Renderer::draw_text(
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
