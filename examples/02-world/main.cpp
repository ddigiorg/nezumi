// =============================================================================
// examples/02-world/main.cpp
//
// This example shows the World's tile-space convention and how to set up a
// camera for it.
//
// - Every simulation position is in tiles: floor(x) is the tile column and
//   the fraction is how far across it (see World/Constants.hpp).
// - Sprites are drawn in tiles too. Their size in the atlas (64x64 px ground
//   tiles, 64x128 px units) has nothing to do with their size in the world.
// - The world camera decides how big a tile is on screen: TILE_PIXELS at
//   zoom 1.0, scaled by the player's zoom.
//
// Controls: W/A/S/D or arrow keys pan, middle mouse drag pans, mouse wheel
// zooms toward the cursor. Press Escape (or close the window) to quit.
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/Core/Random.hpp"
#include "Nezumi/World/Entity/Entity.hpp"
#include "Nezumi/World/WorldContext.hpp"
#include "Nezumi/Input/Events/OnMouseMotion.hpp"
#include "Nezumi/World/Physics/Area2D.hpp"
#include "Nezumi/World/Physics/TileCoord.hpp"
#include "Nezumi/Engine.hpp"
#include <algorithm>   // max, min
#include <array>       // array
#include <cstdint>     // int32_t, uint32_t
#include <cstdio>      // snprintf
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>   // exception
#include <glm/glm.hpp> // vec2, vec4
#include <iostream>    // cerr
#include <memory>      // unique_ptr, make_unique
#include <string>      // to_string
#include <utility>     // swap

using namespace Nezumi; // TODO: should examples be more explicit?

// -----------------------------------------------------------------------------
// Input Actions
//
// Bound to keys in on_init. The engine names no actions: each game (here, each
// example) declares its own.
// -----------------------------------------------------------------------------

namespace Action {
    inline constexpr ActionID PanLeft  = 0;
    inline constexpr ActionID PanRight = 1;
    inline constexpr ActionID PanDown  = 2;
    inline constexpr ActionID PanUp    = 3;
}

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

// Screen pixels per tile at zoom 1.0
static constexpr float TILE_PIXELS = 32.0f;
static constexpr float TILE_FRACTION_1 = 1.0f / TILE_PIXELS;
static constexpr float TILE_FRACTION_2 = 1.0f / TILE_PIXELS;

// Units are spawned in [-SPAWN_RADIUS, SPAWN_RADIUS) tiles on each axis
static constexpr float  SPAWN_RADIUS = 128.0f;
static constexpr size_t UNIT_COUNT   = 20000;

// -----------------------------------------------------------------------------
// Atlas
//
// Pixel boxes in assets/graphics/atlas.png { x0, y0, x1, y1 }, y down.
// -----------------------------------------------------------------------------

namespace AtlasPixels {
    // 64x64 px ground tiles, drawn 1x1 tile
    constexpr glm::vec4 GrassDark  = {  64.0f, 256.0f, 128.0f, 320.0f };
    constexpr glm::vec4 GrassLight = {  64.0f, 320.0f, 128.0f, 384.0f };

    // 64x128 px unit frames, drawn 0.5x1 tile. One row of 5 headings per
    // team: down, down-right, right, up-right, up (left ones are mirrored).
    constexpr glm::vec2 UnitSize = { 64.0f, 128.0f };
    constexpr std::array<glm::vec2, 4> UnitTeamOrigin = {{
        {   0.0f,   0.0f }, // red
        { 320.0f,   0.0f }, // purple
        {   0.0f, 128.0f }, // blue
        { 320.0f, 128.0f }, // teal
    }};
}

// The atlas packs ground tiles edge to edge, so with Linear filtering a tile's
// outermost texels blend with its neighbour's and show up as seams. Sampling
// half a texel inside the box avoids that at every zoom (no mips yet).
constexpr glm::vec4 inset_half_texel(glm::vec4 pixel_box) {
    return pixel_box + glm::vec4{ 0.5f, 0.5f, -0.5f, -0.5f };
}

// Unit sprite size in tiles, anchored on its feet (the unit's position)
static constexpr glm::vec4 UNIT_SPRITE_BOUNDS = { -0.25f, -0.25f, 0.25f, 0.75f };

// -----------------------------------------------------------------------------
// Wanderer
//
// A minimal game-side entity type: wanders around at walking speed, picking a
// new heading about once a second. See 03-unit-combat for engine Units.
// -----------------------------------------------------------------------------

class Wanderer : public Nezumi::Entity {
public:
    // Game-side types start at 100, clear of the engine's (Unit 1, UnitGroup 2)
    static constexpr EntityTypeID TYPE_ID{ 100 };

    static constexpr float SPEED     = 2.0f;  // tiles per second
    static constexpr int   TURN_ODDS = 60;    // 1 in N ticks picks a new heading (~1 s)

    explicit Wanderer(uint8_t team) : team(team) {
        this->set_active();
        this->set_tickable();

        // Half a tile square, centered on the unit's feet
        collision_bounds = { -0.25f, -0.25f, 0.25f, 0.25f };
    }

    void tick(WorldContext& ctx) override {
        if (started_ && ctx.random->get<int>(0, TURN_ODDS - 1) != 0) return;
        started_ = true;

        // One of 8 directions, or standing still. Only velocity is written
        // here: the movement phase moves the unit and updates its chunk.
        Position2D dir = {
            static_cast<float>(ctx.random->get<int>(-1, 1)),
            static_cast<float>(ctx.random->get<int>(-1, 1)),
        };
        if (dir.x != 0.0f && dir.y != 0.0f) dir *= 0.70710678f; // diagonals not faster
        velocity = dir * SPEED;
    }

    uint8_t team = 0;

private:
    bool started_ = false;
};

// -----------------------------------------------------------------------------
// Draw Params
// -----------------------------------------------------------------------------

constexpr Render::DrawParams DRAW_PARAMS_GROUND = {
    .view  = Render::View::World,
    .layer = 0,
};

constexpr Render::DrawParams DRAW_PARAMS_UNITS = {
    .view  = Render::View::World,
    .layer = 1,
};

constexpr Render::DrawParams DRAW_PARAMS_DEBUG = {
    .view  = Render::View::World,
    .layer = 2,
};

constexpr Render::DrawParams DRAW_PARAMS_UI = {
    .view  = Render::View::Screen,
    .layer = 0,
};

// -----------------------------------------------------------------------------
// WorldExample
// -----------------------------------------------------------------------------

class WorldExample final : public IApplication {
public:

    // -------------------------------------------------------------------------
    // OnInit
    // -------------------------------------------------------------------------

    void on_init(Nezumi::Engine& engine) override {
        Nezumi::Logger&   logger   = engine.logger();
        Nezumi::World&    world    = engine.world();
        Render::Renderer2D* renderer = engine.renderer();

        // Setup cameras
        world_camera_  = renderer->get_view_camera(Render::View::World );
        screen_camera_ = renderer->get_view_camera(Render::View::Screen);
        world_camera_->set_pixels_per_unit(TILE_PIXELS);

        // Setup camera controllers
        // Keys that pan the camera
        InputMap& input = engine.input();
        input.bind(Action::PanLeft,  { ScanCode::A, ScanCode::Left  });
        input.bind(Action::PanRight, { ScanCode::D, ScanCode::Right });
        input.bind(Action::PanDown,  { ScanCode::S, ScanCode::Down  });
        input.bind(Action::PanUp,    { ScanCode::W, ScanCode::Up    });

        camera_controller_ = std::make_unique<CameraController>(
            /*.events   = */engine.event_bus(),
            /*.input    = */input,
            /*.renderer = */*renderer,
            /*.camera   = */renderer->get_view(Render::View::World),
            CameraControllerConfig{
                .zoom_init = 1.0f,
                .zoom_min  = 0.125f,
                .zoom_max  = 4.0f,
                .pan_left  = Action::PanLeft,
                .pan_right = Action::PanRight,
                .pan_down  = Action::PanDown,
                .pan_up    = Action::PanUp,
            }
        );

        // The controller tracks the cursor privately; keep our own copy for
        // the "tile under cursor" readout
        cursor_listener_ = engine.event_bus().subscribe<OnMouseMotion>(
            [this](const OnMouseMotion& e) { cursor_ = { e.pos_x, e.pos_y }; }
        );

        // Setup sprite atlas
        atlas_ = renderer->create_atlas(
            Render::AtlasConfig{
                .name     = "world-atlas",
                .filepath = "assets/graphics/atlas.png",
            }
        ).object;

        tex_grass_dark_  = atlas_->tex_box(inset_half_texel(AtlasPixels::GrassDark ));
        tex_grass_light_ = atlas_->tex_box(inset_half_texel(AtlasPixels::GrassLight));

        // Setup font
        font_ = renderer->create_font(
            Render::FontConfig{
                .name     = "world-font",
                .filepath = "assets/fonts/CommitMono-1.143/CommitMono-400-Regular.otf"
            }
        ).object;

        // Pre-create the chunks covering the spawn area so the ground is
        // there even where no unit has walked yet
        const ChunkSpan spawn_chunks = ChunkSpan::span_from_box(
            Area2D(
                /*.min_x = */-SPAWN_RADIUS,
                /*.min_y = */-SPAWN_RADIUS,
                /*.max_x = */ SPAWN_RADIUS,
                /*.max_y = */ SPAWN_RADIUS
            )
        );

        surface_id_ = world.create_surface(
            SurfaceConfig{
                .name           = "surface-0",
                .initial_chunks = spawn_chunks,
            }
        );

        for (size_t i = 0; i < UNIT_COUNT; i++) {
            world.create_entity<Wanderer>(
                /*.surface  = */surface_id_,
                /*.position = */{
                    // TODO: random positions should be from world not engine
                    /*.x = */engine.random().get<float>(-SPAWN_RADIUS, SPAWN_RADIUS),
                    /*.y = */engine.random().get<float>(-SPAWN_RADIUS, SPAWN_RADIUS),
                },
                /*.team     = */static_cast<uint8_t>(i % AtlasPixels::UnitTeamOrigin.size())
            );
        }

        logger.info("World Example");
        logger.info("Press Escape or close the window to exit.");
    }

    // -------------------------------------------------------------------------
    // OnTick
    // -------------------------------------------------------------------------

    void on_tick(Nezumi::Engine& engine) override {}

    // -------------------------------------------------------------------------
    // OnDraw
    // -------------------------------------------------------------------------

    void on_draw(Nezumi::Engine& engine) override {
        Render::Renderer2D* r = engine.renderer();

        // Per frame, not per tick: the camera is presentation, not simulation
        camera_controller_->update(engine.clock().get_real_dt());

        const Nezumi::Surface* surface = engine.world().get_surface(surface_id_);
        if (!surface) return;

        // Only visit chunks the camera can see
        const glm::vec4 view = world_camera_->world_bounds();
        const Area2D    view_area(view.x, view.y, view.z, view.w);
        const ChunkSpan view_chunks = ChunkSpan::span_from_box(view_area);

        surface->each_chunk_in(view_chunks, [&](const Chunk& chunk) {
            draw_ground_(r, chunk, view_area);
            draw_units_ (r, chunk);

            // Chunk borders
            Area2D bounds = chunk.bounds();
            bounds.grow(TILE_FRACTION_1);

            r->draw_rectangle_sdf(
                Render::RectangleSDF{
                    .pos_box      = bounds.to_vec4(),
                    .color        = { 0.0f, 0.0f, 0.0f, 0.0f },
                    .border_color = { 0.0f, 0.0f, 0.0f, 1.0f },
                    .border_width = TILE_FRACTION_2,
                },
                DRAW_PARAMS_DEBUG
            );
        });

        draw_hud_(r, engine);
    }

    // -------------------------------------------------------------------------
    // OnExit
    // -------------------------------------------------------------------------

    void on_exit(Nezumi::Engine& engine) override {
        // Unsubscribe while the event bus is alive
        engine.event_bus().unsubscribe(cursor_listener_);
        camera_controller_.reset();

        Nezumi::Logger& log = engine.logger();
        Nezumi::Engine::LoopStats stats = engine.get_stats();
        log.info("Ran " + std::to_string(stats.tick_count) + " ticks.");
        log.info("Ran " + std::to_string(stats.draw_count) + " draws.");
        log.info("Goodbye!");
    }

private:

    // -------------------------------------------------------------------------
    // Draw Ground
    //
    // One 1x1 tile sprite per visible tile of the chunk, checkerboarded.
    // Every ground sprite shares the layer, order and atlas material, so they
    // all batch into one draw.
    // -------------------------------------------------------------------------

    void draw_ground_(Render::Renderer2D* r, const Chunk& chunk, const Area2D& view) {
        const Area2D& bounds = chunk.bounds();

        // Visible tiles of this chunk, [min, max] inclusive
        const TileCoord min = TileCoord::from_position({
            std::max(bounds.min.x, view.min.x),
            std::max(bounds.min.y, view.min.y),
        });
        const TileCoord max = TileCoord::from_position({
            std::min(bounds.max.x, view.max.x) - 0.001f,
            std::min(bounds.max.y, view.max.y) - 0.001f,
        });

        Render::DrawParams params = DRAW_PARAMS_GROUND;
        params.material = atlas_->material();

        TileCoord tile;
        for (tile.y = min.y; tile.y <= max.y; tile.y++) {
            for (tile.x = min.x; tile.x <= max.x; tile.x++) {
                r->draw_sprite(
                    Render::Sprite{
                        .pos_box = tile.bounds().to_vec4(),
                        .tex_box = ((tile.x + tile.y) & 1) ? tex_grass_light_ : tex_grass_dark_,
                    },
                    params
                );
            }
        }
    }

    // -------------------------------------------------------------------------
    // Draw Units
    //
    // Sorted by y (lower on screen draws on top). The frame faces the unit's
    // heading; left-facing frames are the right-facing ones mirrored.
    // -------------------------------------------------------------------------

    void draw_units_(Render::Renderer2D* r, const Chunk& chunk) {
        Render::DrawParams params = DRAW_PARAMS_UNITS;
        params.material = atlas_->material();

        chunk.visit_active_entities([&](const Nezumi::Entity& entity) {
            if (entity.is_pending_remove())     return true;
            if (!entity.is_type<Wanderer>())    return true;
            const Wanderer& unit = static_cast<const Wanderer&>(entity);

            const glm::vec2 pos = unit.position.to_vec2();
            const glm::vec2 vel = unit.velocity.to_vec2();

            // Heading column: 0 down, 1 down-right, 2 right, 3 up-right, 4 up
            const int dx = (vel.x > 0.0f) - (vel.x < 0.0f);
            const int dy = (vel.y > 0.0f) - (vel.y < 0.0f);
            const int column = (dx == 0) ? (dy > 0 ? 4 : 0) : (dy + 2);

            const glm::vec2 origin = AtlasPixels::UnitTeamOrigin[unit.team]
                                   + glm::vec2{ column * AtlasPixels::UnitSize.x, 0.0f };
            glm::vec4 tex_box = atlas_->tex_box({ origin, origin + AtlasPixels::UnitSize });
            if (dx < 0) std::swap(tex_box.x, tex_box.z); // mirror

            params.order = -static_cast<int32_t>(pos.y * 16.0f);

            r->draw_sprite(
                Render::Sprite{
                    .pos_box = glm::vec4{ pos, pos } + UNIT_SPRITE_BOUNDS,
                    .tex_box = tex_box,
                },
                params
            );
            return true;
        });
    }

    // -------------------------------------------------------------------------
    // Draw HUD
    //
    // Zoom, tile size on screen, and the tile under the cursor split into its
    // integer tile and fraction, which is the tile-space convention in action.
    // -------------------------------------------------------------------------

    void draw_hud_(Render::Renderer2D* r, Nezumi::Engine& engine) {
        const glm::vec2  world = world_camera_->screen_to_world(cursor_);
        const Position2D pos   = { world.x, world.y };
        const TileCoord  tile  = TileCoord::from_position(pos);
        const Position2D frac  = TileCoord::fraction(pos);

        const Nezumi::Engine::LoopStats& stats = engine.get_stats();

        char text[256];
        std::snprintf(text, sizeof text,
            "zoom %.3f (%.1f px/tile)\n"
            "cursor %.2f, %.2f\n"
            "tile %d, %d + %.2f, %.2f\n"
            "units %zu\n"
            "tick %.2f ms  draw %.2f ms\n"
            "objects %u  draw calls %u",
            world_camera_->get_zoom(), world_camera_->get_scale(),
            pos.x, pos.y,
            tile.x, tile.y, frac.x, frac.y,
            engine.world().entity_count(),
            stats.tick_ms, stats.draw_ms,
            r->stats().objects, r->stats().draw_calls
        );

        r->draw_text(
            text,
            { 8.0f, screen_camera_->get_size().y - 8.0f },
            *font_,
            Render::TextStyle {
                .size          = 16.0f,
                .color         = { 1.0f, 1.0f, 1.0f, 1.0f },
                .outline_color = { 0.0f, 0.0f, 0.0f, 1.0f },
                .outline_width = 3.0f,
                .align         = Render::TextAlign::Left,
                .valign        = Render::TextVAlign::Top,
            },
            DRAW_PARAMS_UI
        );
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    // Owned by the renderer. SlotMap objects never move, so these stay valid
    Render::Camera2D* world_camera_  = nullptr;
    Render::Camera2D* screen_camera_ = nullptr;
    Render::Atlas*  atlas_         = nullptr;
    Render::Font*   font_          = nullptr;

    glm::vec4 tex_grass_dark_  = {};
    glm::vec4 tex_grass_light_ = {};

    std::unique_ptr<CameraController> camera_controller_ = nullptr;
    EventListenerID                   cursor_listener_   = {};
    glm::vec2                         cursor_            = { 0.0f, 0.0f }; // window pixels, y down

    SurfaceID surface_id_ = {};
};

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    try {
        Nezumi::EngineConfig config = {};
        config.window.title = "World Example";
        config.escape_exits = true;

        Nezumi::Engine engine(config);

        WorldExample app;
        engine.run(app);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
