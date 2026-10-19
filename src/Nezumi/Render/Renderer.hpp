// =============================================================================
// Renderer.hpp
//
// Engine rendering subsystem. Collects draws in any order during the frame;
// then sorts, batches, uploads, and records them at the end of the frame.
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
//   - Camera origins: Centered (world) and BottomLeft (screen); both keep y up.
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
#include "Nezumi/Render/Atlas/Atlas.hpp"
#include "Nezumi/Render/Atlas/AtlasConfig.hpp"
#include "Nezumi/Render/Frame/BatchBuilder.hpp"
#include "Nezumi/Render/Frame/CommandQueue.hpp"
#include "Nezumi/Render/Frame/FrameBindings.hpp"
#include "Nezumi/Render/Frame/Sort.hpp"
#include "Nezumi/Render/Frame/SortKey.hpp"
#include "Nezumi/Render/Frame/StreamBuffer.hpp"
#include "Nezumi/Render/Kind/KindConcept.hpp"
#include "Nezumi/Render/Kind/KindID.hpp"
#include "Nezumi/Render/Kind/KindRegistry.hpp"
#include "Nezumi/Render/Kinds/Arrow.hpp"
#include "Nezumi/Render/Kinds/ArrowKind.hpp"
#include "Nezumi/Render/Kinds/Circle.hpp"
#include "Nezumi/Render/Kinds/CircleKind.hpp"
#include "Nezumi/Render/Kinds/Line.hpp"
#include "Nezumi/Render/Kinds/LineKind.hpp"
#include "Nezumi/Render/Kinds/Rectangle.hpp"
#include "Nezumi/Render/Kinds/RectangleFilledKind.hpp"
#include "Nezumi/Render/Kinds/RectangleUnfilledKind.hpp"
#include "Nezumi/Render/Kinds/RectangleSDF.hpp"
#include "Nezumi/Render/Kinds/RectangleSDFKind.hpp"
#include "Nezumi/Render/Kinds/Sprite.hpp"
#include "Nezumi/Render/Kinds/SpriteKind.hpp"
#include "Nezumi/Render/Kinds/TextGlyphSDF.hpp"
#include "Nezumi/Render/Kinds/TextGlyphSDFKind.hpp"
#include "Nezumi/Render/Material/MaterialID.hpp"
#include "Nezumi/Render/Material/MaterialTable.hpp"
#include "Nezumi/Render/Shader/ShaderLibrary.hpp"
#include "Nezumi/Render/Text/Font.hpp"
#include "Nezumi/Render/Text/FontConfig.hpp"
#include "Nezumi/Render/Text/Text.hpp"
#include "Nezumi/Render/DrawParams.hpp"
#include "Nezumi/Render/RendererConfig.hpp"
#include "Nezumi/Render/RendererContext.hpp"
#include "Nezumi/Render/View/Camera.hpp"
#include "Nezumi/Render/View/CameraConfig.hpp"
#include "Nezumi/Render/View/CameraOrigin.hpp"
#include "Nezumi/Render/View/ViewID.hpp"
#include "Nezumi/RHI/RHI.hpp"

namespace Nezumi::Render {

class Renderer {
public:

    // -------------------------------------------------------------------------
    // Construction & Destruction
    // -------------------------------------------------------------------------

    explicit Renderer(RHI::Device& device, const RendererConfig& config = {});
    ~Renderer() = default;

    // Non-copyable and non-movable
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&)                 = delete;
    Renderer& operator=(Renderer&&)      = delete;

    // -------------------------------------------------------------------------
    // Frame Control
    //
    // All GPU work happens during end_frame().
    // -------------------------------------------------------------------------

    void begin_frame();
    void end_frame();

    // -------------------------------------------------------------------------
    // Create
    //
    // Creates long-lived resources.
    // -------------------------------------------------------------------------

    SlotMap<Atlas >::Pair create_atlas (const AtlasConfig&  config);
    SlotMap<Camera>::Pair create_camera(const CameraConfig& config);
    SlotMap<Font  >::Pair create_font  (const FontConfig&   config);

    // -------------------------------------------------------------------------
    // Remove
    //
    // Removes long-lived resources.
    // -------------------------------------------------------------------------

    void remove_atlas (Handle<Atlas > handle);
    void remove_camera(Handle<Camera> handle);
    void remove_font  (Handle<Font  > handle);

    // -------------------------------------------------------------------------
    // Get
    //
    // Gets long-lived resources.
    // -------------------------------------------------------------------------

    Atlas*  get_atlas (Handle<Atlas > handle);
    Camera* get_camera(Handle<Camera> handle);
    Font*   get_font  (Handle<Font  > handle);

    RendererContext& get_context() { return context_; }

    // -------------------------------------------------------------------------
    // Views
    //
    // Which camera each view draws through. Each frame, every view's camera is
    // sized to the device's backbuffer and the camera matrix is uploaded to
    // that view's slot.
    //
    // Default views (made by the constructor, unless replaced):
    //
    // - View::World:  "world-camera"  (Centered)
    // - View::Screen: "screen-camera" (BottomLeft, 1 unit = 1 pixel)
    // -------------------------------------------------------------------------

    void           clear_view(ViewID view);
    void           set_view  (ViewID view, Handle<Camera> camera);
    Handle<Camera> get_view  (ViewID view) const;

    // The camera a view draws through, or nullptr if it has none.
    Camera*        get_view_camera(ViewID view);

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
        assert(in_frame_ &&
               "Renderer: drawing outside begin_frame and end_frame");
        assert(params.view < VIEW_CAPACITY && views_[params.view] &&
               "Renderer: drawing to a view with no camera (see set_view)");
        // TODO: assert instead of ensure since kinds should be registered
        //       before any frames
        const KindID kind = kinds_.ensure<K>(context_);
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

    RHI::Device&    device_;
    RendererConfig  config_;
    RendererContext context_;

    // Frame: things rebuilt every frame
    BatchBuilder   builder_;
    FrameBindings  frame_;
    CommandQueue   queue_;
    StreamBuffer   stream_;

    // Resources: long-lived GPU state
    MaterialTable  materials_;
    ShaderLibrary  shaders_;
    KindRegistry   kinds_;

    SlotMap<Atlas > atlases_;
    SlotMap<Camera> cameras_;
    SlotMap<Font  > fonts_;

    std::array<Handle<Camera>, VIEW_CAPACITY> views_ = {};

    // Miscellaneous
    FrameStats stats_;
    bool in_frame_ = false;
};

} // namespace Nezumi::Render
