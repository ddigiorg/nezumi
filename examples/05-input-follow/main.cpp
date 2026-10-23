// =============================================================================
// examples/05-input-follow/main.cpp
//
// This example shows input actions and a camera that follows an entity.
//
// - The example declares its own actions (Move*, Pan*, ToggleFollow) and binds
//   them to keys through the engine's InputMap. Nothing in the engine knows
//   about W/A/S/D.
// - A Walker entity moves in 8 directions. on_tick reads the move axis and
//   hands the Walker a direction between ticks; the Walker turns it into
//   velocity while ticking. Input never reaches the simulation directly.
// - The world camera follows the Walker each frame with an eased catch-up and
//   a small dead zone. Tab switches to a free camera (arrow keys and middle
//   drag pan) and back.
//
// Controls: W/A/S/D move the walker, Tab toggles follow, arrow keys pan the
// free camera, middle mouse drag pans it, mouse wheel zooms. Press Escape (or
// close the window) to quit.
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/World/Entity/Entity.hpp"
#include "Nezumi/World/Physics/Area2D.hpp"
#include "Nezumi/World/Physics/TileCoord.hpp"
#include "Nezumi/World/WorldContext.hpp"
#include "Nezumi/Engine.hpp"
#include <algorithm>   // max, min
#include <array>       // array
#include <cstdint>     // int32_t, uint8_t
#include <cstdio>      // snprintf
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>   // exception
#include <glm/glm.hpp> // vec2, vec4
#include <iostream>    // cerr
#include <memory>      // unique_ptr, make_unique
#include <string>      // to_string
#include <utility>     // swap

using namespace Nezumi;

// -----------------------------------------------------------------------------
// Input Actions
// -----------------------------------------------------------------------------

namespace Action {
    inline constexpr ActionID MoveLeft     = 0;
    inline constexpr ActionID MoveRight    = 1;
    inline constexpr ActionID MoveDown     = 2;
    inline constexpr ActionID MoveUp       = 3;
    inline constexpr ActionID PanLeft      = 4;
    inline constexpr ActionID PanRight     = 5;
    inline constexpr ActionID PanDown      = 6;
    inline constexpr ActionID PanUp        = 7;
    inline constexpr ActionID ToggleFollow = 8;
}

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

// Screen pixels per tile at zoom 1.0
static constexpr float TILE_PIXELS     = 32.0f;
static constexpr float TILE_FRACTION_1 = 1.0f / TILE_PIXELS;

// Ground is pre-created in [-MAP_RADIUS, MAP_RADIUS) tiles on each axis
static constexpr float  MAP_RADIUS     = 64.0f;
static constexpr size_t WANDERER_COUNT = 400;

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

    constexpr uint8_t WalkerTeam = 2; // blue
}

constexpr glm::vec4 inset_half_texel(glm::vec4 pixel_box) {
    return pixel_box + glm::vec4{ 0.5f, 0.5f, -0.5f, -0.5f };
}

// Unit sprite size in tiles, anchored on its feet (the unit's position)
static constexpr glm::vec4 UNIT_SPRITE_BOUNDS = { -0.25f, -0.25f, 0.25f, 0.75f };

// -----------------------------------------------------------------------------
// Walker
//
// The entity the player moves. It never reads input: the app sets move_dir
// between ticks (in on_tick) and the Walker turns it into velocity while
// ticking, the same way Units follow commands.
// -----------------------------------------------------------------------------

class Walker : public Nezumi::Entity {
public:
    static constexpr EntityTypeID TYPE_ID{ 101 };

    static constexpr float SPEED = 4.0f; // tiles per second

    Walker() {
        this->set_active();
        this->set_tickable();
        collision_bounds = { -0.25f, -0.25f, 0.25f, 0.25f };
    }

    void tick(WorldContext&) override {
        velocity = move_dir * SPEED;
        if (!(move_dir == Position2D{})) facing = move_dir;
    }

    Position2D move_dir = {};                // set between ticks, length 0 or 1
    Position2D facing   = { 0.0f, -1.0f };   // last movement direction
};

// -----------------------------------------------------------------------------
// Wanderer
//
// Scenery: walks around so there's something to move past. See 02-world.
// -----------------------------------------------------------------------------

class Wanderer : public Nezumi::Entity {
public:
    static constexpr EntityTypeID TYPE_ID{ 100 };

    static constexpr float SPEED     = 1.5f; // tiles per second
    static constexpr int   TURN_ODDS = 90;   // 1 in N ticks picks a new heading

    explicit Wanderer(uint8_t team) : team(team) {
        this->set_active();
        this->set_tickable();
        collision_bounds = { -0.25f, -0.25f, 0.25f, 0.25f };
    }

    void tick(WorldContext& ctx) override {
        if (started_ && ctx.random->get<int>(0, TURN_ODDS - 1) != 0) return;
        started_ = true;

        Position2D dir = {
            static_cast<float>(ctx.random->get<int>(-1, 1)),
            static_cast<float>(ctx.random->get<int>(-1, 1)),
        };
        if (dir.x != 0.0f && dir.y != 0.0f) dir *= 0.70710678f;
        velocity = dir * SPEED;
    }

    uint8_t team = 0;

private:
    bool started_ = false;
};

// -----------------------------------------------------------------------------
// Draw Params
// -----------------------------------------------------------------------------

constexpr Render::DrawParams DRAW_PARAMS_GROUND = { .view = Render::View::World,  .layer = 0 };
constexpr Render::DrawParams DRAW_PARAMS_UNITS  = { .view = Render::View::World,  .layer = 1 };
constexpr Render::DrawParams DRAW_PARAMS_DEBUG  = { .view = Render::View::World,  .layer = 2 };
constexpr Render::DrawParams DRAW_PARAMS_UI     = { .view = Render::View::Screen, .layer = 0 };

// -----------------------------------------------------------------------------
// InputFollowExample
// -----------------------------------------------------------------------------

class InputFollowExample final : public IApplication {
public:

    // -------------------------------------------------------------------------
    // OnInit
    // -------------------------------------------------------------------------

    void on_init(Nezumi::Engine& engine) override {
        Nezumi::World&    world    = engine.world();
        Render::Renderer* renderer = engine.renderer();

        // Bindings. A settings screen would change these at runtime.
        InputMap& input = engine.input();
        input.bind(Action::MoveLeft,     ScanCode::A);
        input.bind(Action::MoveRight,    ScanCode::D);
        input.bind(Action::MoveDown,     ScanCode::S);
        input.bind(Action::MoveUp,       ScanCode::W);
        input.bind(Action::PanLeft,      ScanCode::Left);
        input.bind(Action::PanRight,     ScanCode::Right);
        input.bind(Action::PanDown,      ScanCode::Down);
        input.bind(Action::PanUp,        ScanCode::Up);
        input.bind(Action::ToggleFollow, ScanCode::Tab);

        // Setup cameras
        world_camera_  = renderer->get_view_camera(Render::View::World );
        screen_camera_ = renderer->get_view_camera(Render::View::Screen);
        world_camera_->set_pixels_per_unit(TILE_PIXELS);

        camera_controller_ = std::make_unique<CameraController>(
            /*.events   = */engine.event_bus(),
            /*.input    = */input,
            /*.renderer = */*renderer,
            /*.camera   = */renderer->get_view(Render::View::World),
            CameraControllerConfig{
                .zoom_init        = 1.5f,
                .zoom_min         = 0.25f,
                .zoom_max         = 4.0f,
                .pan_left         = Action::PanLeft,
                .pan_right        = Action::PanRight,
                .pan_down         = Action::PanDown,
                .pan_up           = Action::PanUp,
                .follow_rate      = 6.0f,
                .follow_dead_zone = { 1.0f, 0.75f }, // tiles
            }
        );

        // Setup sprite atlas and font
        atlas_ = renderer->create_atlas(
            Render::AtlasConfig{
                .name     = "follow-atlas",
                .filepath = "assets/graphics/atlas.png",
            }
        ).object;

        tex_grass_dark_  = atlas_->tex_box(inset_half_texel(AtlasPixels::GrassDark ));
        tex_grass_light_ = atlas_->tex_box(inset_half_texel(AtlasPixels::GrassLight));

        font_ = renderer->create_font(
            Render::FontConfig{
                .name     = "follow-font",
                .filepath = "assets/fonts/CommitMono-1.143/CommitMono-400-Regular.otf"
            }
        ).object;

        // World
        surface_id_ = world.create_surface(
            SurfaceConfig{
                .name           = "surface-0",
                .initial_chunks = ChunkSpan::span_from_box(
                    Area2D(-MAP_RADIUS, -MAP_RADIUS, MAP_RADIUS, MAP_RADIUS)
                ),
            }
        );

        for (size_t i = 0; i < WANDERER_COUNT; i++) {
            world.create_entity<Wanderer>(
                /*.surface  = */surface_id_,
                /*.position = */{
                    engine.random().get<float>(-MAP_RADIUS, MAP_RADIUS),
                    engine.random().get<float>(-MAP_RADIUS, MAP_RADIUS),
                },
                /*.team     = */static_cast<uint8_t>(i % 2 == 0 ? 0 : 3) // red and teal
            );
        }

        walker_ = world.create_entity<Walker>(surface_id_, { 0.5f, 0.5f });

        // Start centered on the walker rather than easing in from the origin
        camera_controller_->set_follow_target({ 0.5f, 0.5f }, /*snap=*/true);

        engine.logger().info("Input Follow Example");
        engine.logger().info("W/A/S/D move, Tab toggles follow, arrows pan the free camera.");
        engine.logger().info("Press Escape or close the window to exit.");
    }

    // -------------------------------------------------------------------------
    // OnTick
    //
    // Between world ticks: read actions and hand the walker its direction.
    // -------------------------------------------------------------------------

    void on_tick(Nezumi::Engine& engine) override {
        const InputMap& input = engine.input();

        if (input.was_pressed(Action::ToggleFollow))
            follow_ = !follow_;

        Walker* walker = engine.world().get_entity_as<Walker>(walker_);
        if (!walker) return;

        // 8 directions; normalized so diagonals aren't faster
        glm::vec2 dir = input.axis2d(
            Action::MoveLeft, Action::MoveRight, Action::MoveDown, Action::MoveUp);
        if (dir != glm::vec2(0.0f)) dir = glm::normalize(dir);

        walker->move_dir = { dir.x, dir.y };
    }

    // -------------------------------------------------------------------------
    // OnDraw
    // -------------------------------------------------------------------------

    void on_draw(Nezumi::Engine& engine) override {
        Render::Renderer* r = engine.renderer();

        // Per frame: the camera is presentation, not simulation
        const Walker* walker = engine.world().get_entity_as<Walker>(walker_);
        if (follow_ && walker) camera_controller_->set_follow_target(walker->position.to_vec2());
        else                   camera_controller_->clear_follow_target();
        camera_controller_->update(engine.clock().get_real_dt());

        const Nezumi::Surface* surface = engine.world().get_surface(surface_id_);
        if (!surface) return;

        const glm::vec4 view = world_camera_->world_bounds();
        const Area2D    view_area(view.x, view.y, view.z, view.w);

        surface->each_chunk_in(ChunkSpan::span_from_box(view_area), [&](const Chunk& chunk) {
            draw_ground_(r, chunk, view_area);
            draw_units_ (r, chunk);
        });

        draw_dead_zone_(r);
        draw_hud_(r, walker);
    }

    // -------------------------------------------------------------------------
    // OnExit
    // -------------------------------------------------------------------------

    void on_exit(Nezumi::Engine& engine) override {
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
    // -------------------------------------------------------------------------

    void draw_ground_(Render::Renderer* r, const Chunk& chunk, const Area2D& view) {
        const Area2D& bounds = chunk.bounds();

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
    // Walker and wanderers share the unit frames. Sorted by y (lower on
    // screen draws on top).
    // -------------------------------------------------------------------------

    void draw_units_(Render::Renderer* r, const Chunk& chunk) {
        Render::DrawParams params = DRAW_PARAMS_UNITS;
        params.material = atlas_->material();

        chunk.visit_active_entities([&](const Nezumi::Entity& entity) {
            if (entity.is_pending_remove()) return true;

            glm::vec2 heading;
            uint8_t   team;
            if (const Walker* w = entity.is_type<Walker>() ? entity.as_type<Walker>() : nullptr) {
                heading = w->facing.to_vec2();
                team    = AtlasPixels::WalkerTeam;
            }
            else if (entity.is_type<Wanderer>()) {
                heading = entity.velocity.to_vec2();
                team    = entity.as_type<Wanderer>()->team;
            }
            else {
                return true;
            }

            const glm::vec2 pos = entity.position.to_vec2();

            // Heading column: 0 down, 1 down-right, 2 right, 3 up-right, 4 up.
            // The 0.38 threshold (~sin 22.5 deg) snaps a heading to 8 ways.
            const int dx = (heading.x > 0.38f) - (heading.x < -0.38f);
            const int dy = (heading.y > 0.38f) - (heading.y < -0.38f);
            const int column = (dx == 0) ? (dy > 0 ? 4 : 0) : (dy + 2);

            const glm::vec2 origin = AtlasPixels::UnitTeamOrigin[team]
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
    // Draw Dead Zone
    //
    // The box around the camera center the walker moves in without the
    // camera following. Only shown while following.
    // -------------------------------------------------------------------------

    void draw_dead_zone_(Render::Renderer* r) {
        if (!camera_controller_->is_following()) return;

        const glm::vec2 center = world_camera_->get_position();
        const glm::vec2 half   = camera_controller_->get_config().follow_dead_zone;

        r->draw_rectangle_sdf(
            Render::RectangleSDF{
                .pos_box      = { center - half, center + half },
                .color        = { 0.0f, 0.0f, 0.0f, 0.0f },
                .border_color = { 1.0f, 1.0f, 1.0f, 0.5f },
                .border_width = TILE_FRACTION_1 * 2.0f,
            },
            DRAW_PARAMS_DEBUG
        );
    }

    // -------------------------------------------------------------------------
    // Draw HUD
    // -------------------------------------------------------------------------

    void draw_hud_(Render::Renderer* r, const Walker* walker) {
        const glm::vec2 pos = walker ? walker->position.to_vec2() : glm::vec2(0.0f);
        const glm::vec2 cam = world_camera_->get_position();

        char text[256];
        std::snprintf(text, sizeof text,
            "camera %s (Tab)\n"
            "walker %.2f, %.2f\n"
            "camera %.2f, %.2f\n"
            "zoom %.2f\n"
            "W/A/S/D move, arrows pan (free camera)",
            camera_controller_->is_following() ? "follow" : "free",
            pos.x, pos.y,
            cam.x, cam.y,
            world_camera_->get_zoom()
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

    Render::Camera* world_camera_  = nullptr;
    Render::Camera* screen_camera_ = nullptr;
    Render::Atlas*  atlas_         = nullptr;
    Render::Font*   font_          = nullptr;

    glm::vec4 tex_grass_dark_  = {};
    glm::vec4 tex_grass_light_ = {};

    std::unique_ptr<CameraController> camera_controller_ = nullptr;

    SurfaceID    surface_id_ = {};
    EntityHandle walker_     = {};
    bool         follow_     = true;
};

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    try {
        Nezumi::EngineConfig config = {};
        config.window.title = "Input Follow Example";
        config.escape_exits = true;

        Nezumi::Engine engine(config);

        InputFollowExample app;
        engine.run(app);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
