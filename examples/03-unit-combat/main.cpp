// =============================================================================
// examples/03-unit-combat/main.cpp
//
// This example shows damageable Units fighting with UnitCommands.
//
// - Four armies, one per faction (and sprite color), attack-move toward the
//   middle of the map and fight whoever they meet. Each unit keeps its offset
//   from its army's center, so armies arrive in formation instead of in a
//   line (a stand-in for what a UnitGroup will do).
// - Units left idle for IDLE_REGROUP_TICKS (survivors whose fight ended, or
//   who arrived with nobody in sight) attack-move back to the middle.
// - Each army mixes two UnitConfigs: melee soldiers and ranged archers. The
//   configs are plain data; every unit just points at one.
// - Units find enemies with Surface::find_nearest_entity and deal damage
//   through World::damage_entity, which is applied after every unit has
//   ticked. A unit at 0 health is removed by the World.
// - A healthbar (black back, white fill) is drawn over every unit.
//
// Controls: right click sends every unit attack-moving to the cursor, R
// restarts the battle. W/A/S/D or arrow keys pan, middle mouse drag pans,
// mouse wheel zooms toward the cursor. Press Escape (or close the window) to
// quit.
// =============================================================================

#include "Nezumi/Control/CameraController.hpp"
#include "Nezumi/Engine.hpp"
#include "Nezumi/Input/Events/OnKeyDown.hpp"
#include "Nezumi/Input/Events/OnMouseButtonDown.hpp"
#include "Nezumi/Input/Events/OnMouseMotion.hpp"
#include "Nezumi/Render/Kinds/Rectangle.hpp"
#include "Nezumi/World/Entities/Unit.hpp"
#include "Nezumi/World/Entities/UnitConfig.hpp"
#include "Nezumi/World/Physics/Area2D.hpp"
#include "Nezumi/World/Physics/TileCoord.hpp"
#include <algorithm>   // max, min
#include <array>       // array
#include <cstdint>     // int32_t, uint8_t
#include <cstdio>      // snprintf
#include <cstdlib>     // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>   // exception
#include <glm/glm.hpp> // vec2, vec4
#include <iostream>    // cerr
#include <memory>      // unique_ptr, make_unique
#include <optional>    // optional
#include <string>      // to_string
#include <utility>     // swap
#include <vector>      // vector

using namespace Nezumi;

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

// Screen pixels per tile at zoom 1.0
static constexpr float TILE_PIXELS = 32.0f;
static constexpr float TILE_FRACTION_1 = 1.0f / TILE_PIXELS;

// Armies spawn in a square of ARMY_HALF_SIZE tiles around their corner,
// ARMY_DISTANCE tiles from the middle on each axis
static constexpr size_t FACTION_COUNT  = 4;
static constexpr size_t ARMY_SIZE      = 300;
static constexpr float  ARMY_DISTANCE  = 28.0f;
static constexpr float  ARMY_HALF_SIZE = 8.0f;
static constexpr size_t ARCHER_EVERY   = 4; // every 4th unit is an archer

static constexpr float MAP_RADIUS = 64.0f; // ground pre-created in [-R, R)

// Idle units with no target regroup at the middle after this long
static constexpr uint16_t   IDLE_REGROUP_TICKS = 180; // 3 s at 60 UPS
static constexpr Position2D REGROUP_POSITION   = { 0.0f, 0.0f };

// -----------------------------------------------------------------------------
// Unit Configs
//
// Shared stats, one per kind of unit. Static storage, so they outlive every
// unit that points at them.
// -----------------------------------------------------------------------------

static constexpr UnitConfig SOLDIER = {
    .name            = "soldier",
    .subtype         = EntitySubtypeID{ 0 },
    .max_health      = 100,
    .move_speed      = 4.0f,  // tiles per second
    .attack_damage   = 10,
    .attack_interval = 60,    // ticks (1 hit per second)
    .attack_range    = 0.75f, // tile
    .aggro_radius    = 6.0f,  // tiles
};

static constexpr UnitConfig ARCHER = {
    .name            = "archer",
    .subtype         = EntitySubtypeID{ 1 },
    .max_health      = 60,
    .move_speed      = 2.0f,
    .attack_damage   = 6,
    .attack_interval = 45,
    .attack_range    = 4.0f,
    .aggro_radius    = 7.0f,
};

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
    // color: down, down-right, right, up-right, up (left ones are mirrored).
    constexpr glm::vec2 UnitSize = { 64.0f, 128.0f };
}

// One color per faction: FactionIndex indexes these arrays directly
struct FactionStyle {
    const char* name;
    glm::vec2   atlas_origin; // first unit frame in the atlas
};

static constexpr std::array<FactionStyle, FACTION_COUNT> FACTIONS = {{
    { "red",    {   0.0f,   0.0f } },
    { "purple", { 320.0f,   0.0f } },
    { "blue",   {   0.0f, 128.0f } },
    { "teal",   { 320.0f, 128.0f } },
}};

// Army corners, same order as FACTIONS
static constexpr std::array<glm::vec2, FACTION_COUNT> ARMY_CORNERS = {{
    { -1.0f,  1.0f },
    {  1.0f,  1.0f },
    { -1.0f, -1.0f },
    {  1.0f, -1.0f },
}};

constexpr glm::vec4 inset_half_texel(glm::vec4 pixel_box) {
    return pixel_box + glm::vec4{ 0.5f, 0.5f, -0.5f, -0.5f };
}

// Unit sprite size in tiles, anchored on its feet (the unit's position)
static constexpr glm::vec4 UNIT_SPRITE_BOUNDS = { -0.25f, -0.25f, 0.25f, 0.75f };

// Healthbar above the sprite, in tiles relative to the unit's position. The
// back is the full bar plus its border; the fill shrinks with health.
static constexpr glm::vec4 HEALTHBAR_BOUNDS = { -0.3f, 0.82f, 0.3f, 0.97f };
static constexpr float     HEALTHBAR_BORDER = 0.04f;

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

// Above every unit, and order 0 so all bars batch into one draw. Within the
// same layer and order, submission order holds: back first, then fill.
constexpr Render::DrawParams DRAW_PARAMS_HEALTHBARS = {
    .view  = Render::View::World,
    .layer = 2,
};

constexpr Render::DrawParams DRAW_PARAMS_UI = {
    .view  = Render::View::Screen,
    .layer = 0,
};

// -----------------------------------------------------------------------------
// UnitCombatExample
// -----------------------------------------------------------------------------

class UnitCombatExample final : public IApplication {
public:

    // -------------------------------------------------------------------------
    // OnInit
    // -------------------------------------------------------------------------

    void on_init(Nezumi::Engine& engine) override {
        Nezumi::World&    world    = engine.world();
        Render::Renderer* renderer = engine.renderer();

        // Setup cameras
        world_camera_  = renderer->get_view_camera(Render::View::World );
        screen_camera_ = renderer->get_view_camera(Render::View::Screen);
        world_camera_->set_pixels_per_unit(TILE_PIXELS);

        camera_controller_ = std::make_unique<CameraController>(
            /*.events   = */engine.event_bus(),
            /*.renderer = */*renderer,
            /*.camera   = */renderer->get_view(Render::View::World),
            CameraControllerConfig{
                .zoom_init = 0.5f,
                .zoom_min  = 0.125f,
                .zoom_max  = 4.0f,
            }
        );

        // Input. Handlers only record what happened; on_tick applies it
        // between world ticks, which is where unit commands belong.
        listeners_.push_back(engine.event_bus().subscribe<OnMouseMotion>(
            [this](const OnMouseMotion& e) { cursor_ = { e.pos_x, e.pos_y }; }
        ));
        listeners_.push_back(engine.event_bus().subscribe<OnMouseButtonDown>(
            [this](const OnMouseButtonDown& e) {
                if (e.button != MouseButton::Right) return;
                const glm::vec2 p = world_camera_->screen_to_world(cursor_);
                pending_move_ = Position2D{ p.x, p.y };
            }
        ));
        listeners_.push_back(engine.event_bus().subscribe<OnKeyDown>(
            [this](const OnKeyDown& e) {
                if (e.scancode == ScanCode::R && !e.repeat) pending_reset_ = true;
            }
        ));

        // Setup sprite atlas and font
        atlas_ = renderer->create_atlas(
            Render::AtlasConfig{
                .name     = "combat-atlas",
                .filepath = "assets/graphics/atlas.png",
            }
        ).object;

        tex_grass_dark_  = atlas_->tex_box(inset_half_texel(AtlasPixels::GrassDark ));
        tex_grass_light_ = atlas_->tex_box(inset_half_texel(AtlasPixels::GrassLight));

        font_ = renderer->create_font(
            Render::FontConfig{
                .name     = "combat-font",
                .filepath = "assets/fonts/CommitMono-1.143/CommitMono-400-Regular.otf"
            }
        ).object;

        surface_id_ = world.create_surface(
            SurfaceConfig{
                .name           = "battlefield",
                .initial_chunks = ChunkSpan::span_from_box(
                    Area2D(-MAP_RADIUS, -MAP_RADIUS, MAP_RADIUS, MAP_RADIUS)),
            }
        );

        spawn_armies_(engine);

        engine.logger().info("Unit Combat Example");
        engine.logger().info("Right click: attack-move every unit. R: restart.");
        engine.logger().info("Press Escape or close the window to exit.");
    }

    // -------------------------------------------------------------------------
    // OnTick
    //
    // Runs before World::tick, so commands issued here take effect this tick.
    // -------------------------------------------------------------------------

    void on_tick(Nezumi::Engine& engine) override {
        if (pending_reset_) {
            pending_reset_ = false;
            for (EntityHandle handle : units_) engine.world().remove_entity(handle);
            units_.clear();
            spawn_armies_(engine);
        }

        if (pending_move_) {
            command_formation_move_(engine.world(), *pending_move_);
            pending_move_.reset();
        }

        // Idle means no target in aggro range: nothing left to fight here
        for_each_unit_(engine.world(), [&](Unit& unit) {
            if (unit.state() != UnitState::Idle)           return;
            if (unit.state_ticks() < IDLE_REGROUP_TICKS)   return;
            if (unit.at(REGROUP_POSITION))                 return; // already there
            unit.attack_move(REGROUP_POSITION);
        });
    }

    // -------------------------------------------------------------------------
    // OnDraw
    // -------------------------------------------------------------------------

    void on_draw(Nezumi::Engine& engine) override {
        Render::Renderer* r = engine.renderer();

        camera_controller_->update(engine.clock().get_real_dt());

        const Nezumi::Surface* surface = engine.world().get_surface(surface_id_);
        if (!surface) return;

        // Only visit chunks the camera can see
        const glm::vec4 view = world_camera_->world_bounds();
        const Area2D    view_area(view.x, view.y, view.z, view.w);
        const ChunkSpan chunk_span = ChunkSpan::span_from_box(view_area);

        surface->each_chunk_in(chunk_span, [&](const Chunk& chunk) {
            draw_ground_(r, chunk, view_area);
            draw_units_ (r, chunk);
        });

        draw_hud_(r, engine);
    }

    // -------------------------------------------------------------------------
    // OnExit
    // -------------------------------------------------------------------------

    void on_exit(Nezumi::Engine& engine) override {
        for (EventListenerID id : listeners_) engine.event_bus().unsubscribe(id);
        camera_controller_.reset();

        engine.logger().info("Ran " + std::to_string(engine.get_stats().tick_count) + " ticks.");
        engine.logger().info("Goodbye!");
    }

private:

    // -------------------------------------------------------------------------
    // Spawn Armies
    //
    // One army per faction in its corner, attack-moving to the middle.
    // FactionIndex i is both the faction bit and the sprite color.
    // -------------------------------------------------------------------------

    void spawn_armies_(Nezumi::Engine& engine) {
        Nezumi::World& world = engine.world();

        for (FactionIndex f = 0; f < FACTION_COUNT; f++) {
            const glm::vec2 center = ARMY_CORNERS[f] * ARMY_DISTANCE;

            for (size_t i = 0; i < ARMY_SIZE; i++) {
                const UnitConfig& config = (i % ARCHER_EVERY == 0) ? ARCHER : SOLDIER;

                // Archers start at the back of the army, away from the middle
                const glm::vec2 back = ARMY_CORNERS[f] * (&config == &ARCHER ? ARMY_HALF_SIZE : 0.0f);

                const Position2D position = {
                    center.x + back.x + engine.random().get<float>(-ARMY_HALF_SIZE, ARMY_HALF_SIZE),
                    center.y + back.y + engine.random().get<float>(-ARMY_HALF_SIZE, ARMY_HALF_SIZE),
                };

                const EntityHandle handle = world.create_entity<Unit>(
                    /*.surface  = */surface_id_,
                    /*.position = */position,
                    /*.config   = */config,
                    /*.factions = */faction_bit(f)
                );

                // The unit exists now (it joins the world next tick), so it
                // can already take a command. Same offset from the middle of
                // the map as from its army's center.
                const Position2D offset = position - Position2D{ center.x, center.y };
                world.get_entity_as<Unit>(handle)->attack_move(offset);
                units_.push_back(handle);
            }
        }
    }

    // Attack-move every unit to `destination`, keeping each unit's offset from
    // its faction's center so armies keep their shape.
    void command_formation_move_(Nezumi::World& world, Position2D destination) {
        std::array<Position2D, FACTION_COUNT> sum   = {};
        std::array<float,      FACTION_COUNT> count = {};

        for_each_unit_(world, [&](Unit& unit) {
            sum  [unit.faction_index()] += unit.position;
            count[unit.faction_index()] += 1.0f;
        });

        for_each_unit_(world, [&](Unit& unit) {
            const FactionIndex f      = unit.faction_index();
            const Position2D   center = sum[f] / count[f];
            unit.attack_move(destination + (unit.position - center));
        });
    }

    // Every unit we spawned that hasn't died, including ones that join the
    // world next tick (not is_alive() yet, but they can take commands). Dead
    // units are dropped here: their handles resolve to nullptr, or they are
    // pending removal.
    template <typename Fn>
    void for_each_unit_(Nezumi::World& world, Fn&& fn) {
        std::erase_if(units_, [&](EntityHandle handle) {
            Unit* unit = world.get_entity_as<Unit>(handle);
            if (!unit || unit->is_pending_remove()) return true;
            fn(*unit);
            return false;
        });
    }

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
    // Sprite faces Unit::facing() (movement or attack direction), sorted by y.
    // A healthbar is drawn over each unit.
    //
    // FUTURE: an Animation class picks the frame from state() and facing().
    // -------------------------------------------------------------------------

    void draw_units_(Render::Renderer* r, const Chunk& chunk) {
        Render::DrawParams params = DRAW_PARAMS_UNITS;
        params.material = atlas_->material();

        chunk.visit_active_entities([&](const Nezumi::Entity& entity) {
            if (!entity.is_alive())      return true;
            if (!entity.is_type<Unit>()) return true;
            const Unit& unit = *entity.as_type<Unit>();

            const glm::vec2 pos = unit.position.to_vec2();

            // Heading column: 0 down, 1 down-right, 2 right, 3 up-right, 4 up.
            // 0.383 = sin(22.5 deg) splits the circle into 8 equal sectors.
            const glm::vec2 facing = unit.facing().to_vec2();
            const int dx = (facing.x > 0.383f) - (facing.x < -0.383f);
            const int dy = (facing.y > 0.383f) - (facing.y < -0.383f);
            const int column = (dx == 0) ? (dy > 0 ? 4 : 0) : (dy + 2);

            const glm::vec2 origin = FACTIONS[unit.faction_index()].atlas_origin
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

            draw_healthbar_(r, pos, unit.health_ratio());
            return true;
        });
    }

    void draw_healthbar_(Render::Renderer* r, glm::vec2 pos, float ratio) {
        const glm::vec4 back = glm::vec4{ pos, pos } + HEALTHBAR_BOUNDS;

        glm::vec4 fill = back + glm::vec4{ HEALTHBAR_BORDER, HEALTHBAR_BORDER,
                                          -HEALTHBAR_BORDER, -HEALTHBAR_BORDER };
        fill.z = fill.x + (fill.z - fill.x) * ratio;

        r->draw_rectangle(
            Render::Rectangle{
                .pos_box = back,
                .color   = { 0.0f, 0.0f, 0.0f, 1.0f },
            },
            DRAW_PARAMS_HEALTHBARS
        );

        r->draw_rectangle(
            Render::Rectangle{
                .pos_box = fill,
                .color   = { 1.0f, 1.0f, 1.0f, 1.0f },
            },
            DRAW_PARAMS_HEALTHBARS
        );
    }

    // -------------------------------------------------------------------------
    // Draw HUD
    //
    // Units left per faction. FactionIndex indexes the counts directly.
    // -------------------------------------------------------------------------

    void draw_hud_(Render::Renderer* r, Nezumi::Engine& engine) {
        std::array<size_t, FACTION_COUNT> alive = {};

        engine.world().each_entity([&](const Nezumi::Entity& entity) {
            if (entity.is_type<Unit>() && entity.is_alive()) alive[entity.faction_index()]++;
        });

        const Nezumi::Engine::LoopStats& stats = engine.get_stats();

        char text[512];
        std::snprintf(text, sizeof text,
            "%-6s %4zu\n%-6s %4zu\n%-6s %4zu\n%-6s %4zu\n"
            "tick %.2f ms  draw %.2f ms\n"
            "right click: attack-move  R: restart",
            FACTIONS[0].name, alive[0], FACTIONS[1].name, alive[1],
            FACTIONS[2].name, alive[2], FACTIONS[3].name, alive[3],
            stats.tick_ms, stats.draw_ms
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
    std::vector<EventListenerID>      listeners_         = {};
    glm::vec2                         cursor_            = { 0.0f, 0.0f }; // window pixels, y down

    // Input waiting for the next on_tick
    std::optional<Position2D> pending_move_  = {};
    bool                      pending_reset_ = false;

    SurfaceID                 surface_id_ = {};
    std::vector<EntityHandle> units_      = {};
};

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    try {
        Nezumi::EngineConfig config = {};
        config.window.title = "Unit Combat Example";
        config.escape_exits = true;

        Nezumi::Engine engine(config);

        UnitCombatExample app;
        engine.run(app);

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
