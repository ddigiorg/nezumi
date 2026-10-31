// =============================================================================
// Renderer2D.hpp
//
// The batched 2D feature renderer: sprites, shapes and SDF text. Collects
// draws in any order during the frame; then sorts, batches, uploads, and
// records them when RenderCore ends the frame.
//
// It is one IRenderFeature of RenderCore. RenderCore owns the frame and the
// passes (FrameRecipe); Renderer2D records its batches into whichever passes
// list it, limited to each pass's views. It never begins a pass or clears.
//
//   RenderCore core(device);
//   Renderer2D r2d(core);  // registers itself with core
//
// Shared services (materials, shaders, the instance stream, the per-view
// uniforms) belong to RenderCore, so another feature (e.g. a mesh renderer)
// can use the same atlases' materials and views.
//
// ## Features
//
// Sort & batch pipeline
//
//   - Every draw gets a 64-bit sort key:
//     | view 4 | layer 8 | order 24 | kind 8 | material 16 | spare 4 |
//   - Painter's order comes first (view -> layer -> order). Within equal
//     painter's order, draws are grouped by GPU state (kind -> material),
//     so they batch automatically without the caller arranging them.
//   - Stable LSD radix sort that skips byte passes where all keys agree.
//   - Adjacent items with the same view/kind/material merge into one
//     instanced draw call, even across layers.
//
// Instance data
//
//   - Draws are appended to a CPU arena (16-byte aligned, no allocations
//     after warm-up), then gathered into a chunked GPU StreamBuffer.
//   - One buffer write per chunk per frame; each run starts 16-byte aligned,
//     with instances tightly packed at their stride.
//
// Item kinds (pluggable)
//
//   - A kind is a traits struct. Adding one doesn't touch the core.
//   - KindRegistry creates pipelines lazily; kinds_.ensure<K>() builds them at
//     startup so broken shaders fail early.
//
// Cameras & views
//
//   - Up to 16 views (View::World, View::Screen, ...), each bound to a
//     camera. Cameras are long-lived and owned in a SlotMap. A view refers
//     to a camera, never the other way round, so one camera can serve
//     several views.
//   - Per-view matrices live in one uniform buffer, selected with a dynamic
//     offset: one group-0 bind per view per frame.
//   - Camera2D origins: Centered (world) and BottomLeft (screen); both keep y up.
//   - A game-side projection (e.g. top-down or 2:1 isometric) maps world to
//     view space on the CPU, before submission; cameras and kinds only see view
//     space. Quad draws square textures as projected diamonds.
//
// Materials & resources
//
//   - MaterialTable owns only bind groups (texture + sampler).
//   - SpriteAtlas and Font own their textures and samplers, and clean up
//     on failure.
//   - SpriteAtlas loads one image with named regions, per-sprite boxes
//     and pivots.
//   - Font uses a FreeType SDF glyph atlas with fallback glyphs; text
//     layout supports alignment and measurement.
//
// ## Future Improvements
//
// 1.  Texture2DArray atlases. Put all atlas pages in one array texture, with
//     the page index in the instance data. Material switches disappear for
//     sprites, which fixes the worst-case draw-call explosion.
// 2.  View-space culling. Skip off-screen objects before submission
//     (spatial grid / chunk lists on the game side).
// 3.  Retained static layers. Cache tile / terrain instance buffers and
//     re-upload only when they change, instead of resubmitting each frame.
// 4.  GPU timing. Add GL_TIME_ELAPSED queries per pass; the CPU stopwatch
//     can't see overdraw or fill-rate cost.
// 5.  Persistently mapped, triple-buffered stream buffer. Avoids implicit
//     syncs when writing a chunk the GPU is still reading.
// 6.  RGBA8 instance colors. Shrinks Sprite from 48 to 36 bytes (less
//     bandwidth); unpack in the shader.
// 7.  Mipmaps. Generate mips and pad/extrude sprite borders to stop
//     bleeding (currently restricted to mip_levels = 1; see TODO in
//     AtlasConfig.hpp).
// 8.  Atlas builder. Pack loose images into pages offline, grouping
//     sprites that are drawn together.
// 9.  Faster font loading. Bitmap -> SDF path (bsdf), multithreaded glyph
//     rendering, and an on-disk atlas cache.
// 10. Non-instanced kinds (DrawMode). Support kinds with arbitrary vertex
//     data, e.g. meshes, polylines and particles.
// 11. Multithreaded submission. Per-thread CommandQueues merged before
//     the sort.
// 12. Virtual texturing. Stream atlas pages on demand for very large sprite
//     sets (Factorio-style), once VRAM becomes the limit.
// =============================================================================

#pragma once
#include "Nezumi/Object/SlotMap.hpp"
#include "Nezumi/Render/2D/Atlas/Atlas.hpp"
#include "Nezumi/Render/2D/Atlas/AtlasConfig.hpp"
#include "Nezumi/Render/2D/Batch/BatchBuilder.hpp"
#include "Nezumi/Render/2D/Batch/SortKey.hpp"
#include "Nezumi/Render/2D/Camera/Camera2D.hpp"
#include "Nezumi/Render/2D/Camera/CameraConfig.hpp"
#include "Nezumi/Render/2D/Camera/CameraOrigin.hpp"
#include "Nezumi/Render/2D/Kind/KindConcept.hpp"
#include "Nezumi/Render/2D/Kind/KindID.hpp"
#include "Nezumi/Render/2D/Kind/KindRegistry.hpp"
#include "Nezumi/Render/2D/Kinds/Arrow.hpp"
#include "Nezumi/Render/2D/Kinds/ArrowKind.hpp"
#include "Nezumi/Render/2D/Kinds/Circle.hpp"
#include "Nezumi/Render/2D/Kinds/CircleKind.hpp"
#include "Nezumi/Render/2D/Kinds/Line.hpp"
#include "Nezumi/Render/2D/Kinds/LineKind.hpp"
#include "Nezumi/Render/2D/Kinds/Quad.hpp"
#include "Nezumi/Render/2D/Kinds/QuadKind.hpp"
#include "Nezumi/Render/2D/Kinds/Rectangle.hpp"
#include "Nezumi/Render/2D/Kinds/RectangleFilledKind.hpp"
#include "Nezumi/Render/2D/Kinds/RectangleSDF.hpp"
#include "Nezumi/Render/2D/Kinds/RectangleSDFKind.hpp"
#include "Nezumi/Render/2D/Kinds/RectangleUnfilledKind.hpp"
#include "Nezumi/Render/2D/Kinds/Sprite.hpp"
#include "Nezumi/Render/2D/Kinds/SpriteKind.hpp"
#include "Nezumi/Render/2D/Kinds/TextGlyphSDF.hpp"
#include "Nezumi/Render/2D/Kinds/TextGlyphSDFKind.hpp"
#include "Nezumi/Render/2D/Kinds/UIQuad.hpp"
#include "Nezumi/Render/2D/Kinds/UIQuadKind.hpp"
#include "Nezumi/Render/2D/DrawParams.hpp"
#include "Nezumi/Render/2D/Renderer2DConfig.hpp"
#include "Nezumi/Render/Core/Frame/CommandQueue.hpp"
#include "Nezumi/Render/Core/Frame/IRenderFeature.hpp"
#include "Nezumi/Render/Core/Material/MaterialID.hpp"
#include "Nezumi/Render/Core/Text/Font.hpp"
#include "Nezumi/Render/Core/Text/FontConfig.hpp"
#include "Nezumi/Render/Core/Text/Text.hpp"
#include "Nezumi/Render/Core/View/ViewID.hpp"
#include "Nezumi/Render/Core/RenderCore.hpp"
#include "Nezumi/RHI/RHI.hpp"

namespace Nezumi::Render {

class Renderer2D final : public IRenderFeature {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    // Registers with `core` (core.add_feature); the destructor unregisters.
    // Add it to a pass of core's FrameRecipe to see its draws.
    explicit Renderer2D(RenderCore& core, const Renderer2DConfig& config = {});
    ~Renderer2D() override;

    // Non-copyable and non-movable
    Renderer2D(const Renderer2D&)            = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;
    Renderer2D(Renderer2D&&)                 = delete;
    Renderer2D& operator=(Renderer2D&&)      = delete;

    // -------------------------------------------------------------------------
    // Frame Hooks (IRenderFeature)
    //
    // Called by RenderCore::begin_frame() and end_frame(); not by the game.
    // -------------------------------------------------------------------------

    std::string_view name() const override { return "renderer2d"; }

    void begin_frame(RenderCore& core) override;
    void prepare    (RenderCore& core) override;
    void upload     (RenderCore& core) override;
    void record     (RenderCore& core, RHI::CommandEncoder& encoder, ViewMask views) override;
    void finish     (RenderCore& core) override;

    // -------------------------------------------------------------------------
    // Create
    //
    // Creates long-lived resources.
    // -------------------------------------------------------------------------

    SlotMap<Atlas >::Pair create_atlas (const AtlasConfig&  config);
    SlotMap<Camera2D>::Pair create_camera(const CameraConfig& config);
    SlotMap<Font  >::Pair create_font  (const FontConfig&   config);

    // -------------------------------------------------------------------------
    // Remove
    //
    // Removes long-lived resources.
    // -------------------------------------------------------------------------

    void remove_atlas (Handle<Atlas > handle);
    void remove_camera(Handle<Camera2D> handle);
    void remove_font  (Handle<Font  > handle);

    // -------------------------------------------------------------------------
    // Get
    //
    // Gets long-lived resources.
    // -------------------------------------------------------------------------

    Atlas*  get_atlas (Handle<Atlas > handle);
    Camera2D* get_camera(Handle<Camera2D> handle);
    Font*   get_font  (Handle<Font  > handle);

    RenderCore& core() { return core_; }

    // -------------------------------------------------------------------------
    // Views
    //
    // Which camera each view draws through. Each frame, every view's camera is
    // sized to the device's backbuffer, given the display scale, and its
    // matrices are published to that view's slot (RenderCore::set_view).
    //
    // Default views (made by the constructor, unless replaced):
    //
    // - View::World:  "world-camera"  (Centered, Renderer2DConfig::world_pixels_per_unit)
    // - View::Screen: "screen-camera" (BottomLeft, 1 unit = 1 point, which is
    //                                  display_scale pixels)
    //
    // The display scale is the screen's HiDPI factor (RenderCore's, set by
    // the Engine before each frame), so content keeps its physical size while
    // rendering at the screen's full resolution.
    // -------------------------------------------------------------------------

    float get_display_scale() const { return core_.get_display_scale(); }

    void             clear_view(ViewID view);
    void             set_view  (ViewID view, Handle<Camera2D> camera);
    Handle<Camera2D> get_view  (ViewID view) const;

    // The camera a view draws through, or nullptr if it has none.
    Camera2D*        get_view_camera(ViewID view);

    // -------------------------------------------------------------------------
    // Render
    //
    // Draws per-frame render objects (lines, rectangles, sprites, text, etc.).
    // -------------------------------------------------------------------------

    // Render generic render object
    template <KindConcept K>
    void draw(
        const typename K::Instance& instance,
        const DrawParams&           params = {})
    {
        assert(core_.in_frame() &&
               "Renderer2D: drawing outside RenderCore::begin_frame and end_frame");
        assert(params.view < VIEW_CAPACITY && views_[params.view] &&
               "Renderer2D: drawing to a view with no camera (see set_view)");
        // TODO: assert instead of ensure since kinds should be registered
        //       before any frames
        const KindID kind = kinds_.ensure<K>(core_.context());
        queue_.push(SortKey::make(params, kind), instance);
    }

    // Render Arrow render object
    void draw_arrow(
        const Arrow&      arrow,
        const DrawParams& params = {});

    // Render Circle render object
    void draw_circle(
        const Circle&     circle,
        const DrawParams& params = {},
        const bool        filled = true);

    // Render Line render object
    void draw_line(
        const Line&       line,
        const DrawParams& params = {});

    // Render Quad render object (textured, 4 free corners; see Quad.hpp)
    void draw_quad(
        const Quad&       quad,
        const DrawParams& params = {});

    // Render Rectangle render object
    void draw_rectangle(
        const Rectangle&  rectangle,
        const DrawParams& params = {},
        const bool        filled = true);

    // Render RectangleSDF render object
    void draw_rectangle_sdf(
        const RectangleSDF& rectangle,
        const DrawParams&   params = {});

    // Render Sprite render object
    void draw_sprite(
        const Sprite&     sprite,
        const DrawParams& params = {});

    // Render text using TextGlyph render objects
    void draw_text(
        std::string_view text,
        glm::vec2        position,
        const Font&      font,
        const TextStyle& style = {},
        DrawParams       params = {});

    // -------------------------------------------------------------------------
    // Frame Statistics
    // -------------------------------------------------------------------------

    struct FrameStats {
        std::uint32_t objects           = 0; // render objects submitted to draw
        std::uint32_t draw_calls        = 0; // batch runs recorded
        std::uint32_t pipeline_switches = 0;
        std::uint32_t material_switches = 0;
        std::uint32_t view_switches     = 0;
        std::uint32_t dropped           = 0; // render objects that didn't fit in GPU memory
        std::size_t   stream_bytes      = 0; // instance bytes uploaded
        std::size_t   stream_chunks     = 0; // GPU chunks allocated so far
    };

    // Stats for the last completed frame.
    const FrameStats& stats() const { return stats_; }

private:

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    RenderCore&      core_;
    Renderer2DConfig config_;

    // Frame: things rebuilt every frame
    BatchBuilder               builder_;
    CommandQueue               queue_;
    std::span<const DrawBatch> batches_; // built in prepare(), recorded in record()

    // Resources: long-lived GPU state
    KindRegistry kinds_;

    SlotMap<Atlas   > atlases_;
    SlotMap<Camera2D> cameras_;
    SlotMap<Font    > fonts_;

    std::array<Handle<Camera2D>, VIEW_CAPACITY> views_ = {};

    // Miscellaneous
    FrameStats stats_;
};

} // namespace Nezumi::Render
